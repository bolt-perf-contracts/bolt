local mg     = require "moongen"
local memory = require "memory"
local device = require "device"
local ts     = require "timestamping"
local filter = require "filter"
local hist   = require "histogram"
local stats  = require "stats"
local timer  = require "timer"
local log    = require "log"

-- set addresses here
local SRC_MAC_BASE      = 1
local DST_MAC_BASE	= 68000 -- Greater than Max(nflws)
local SRC_IP_BASE	= "10.0.0.1" -- actual address will be SRC_IP_BASE + random(0, flows)
local DST_IP		= "192.168.4.10"
local SRC_PORT		= 234
local DST_PORT		= 319

function configure(parser)
	parser:description("Generates UDP traffic and measure latencies. Edit the source to modify constants like IPs.")
	parser:argument("txDev", "Device to transmit from."):convert(tonumber)
	parser:argument("rxDev", "Device to receive from."):convert(tonumber)
	parser:option("-r --rate", "Transmit rate in Mbit/s."):default(10000):convert(tonumber)
	parser:option("-f --flows", "Number of flows (randomized source IP)."):default(4):convert(tonumber)
	parser:option("-s --size", "Packet size."):default(60):convert(tonumber)
	parser:option("-u --upheat", "Heatup time before beginning of latency measurement"):default(2):convert(tonumber)
	parser:option("-t --timeout", "Time to run the test"):default(0):convert(tonumber)
end

function setRate(queue, packetSize, rate_mbps)
	queue:setRate(rate_mbps - (packetSize + 4) * 8 / 1000);
end

function master(args)
	txDev = device.config{port = args.txDev, rxQueues = 3, txQueues = 3}
	rxDev = device.config{port = args.rxDev, rxQueues = 3, txQueues = 3}
	device.waitForLinks()
	local file = io.open("mf-find-mg-1p.txt", "w")
	file:write("#flows rate #pkt #pkt/s loss\n")
	local maxRate = args.rate
	if maxRate <= 0 then
		maxRate = 10000
	end
	local minRate = 10
	local rate = minRate + (maxRate - minRate)/2
 	for _,nflws in pairs({50000,60000}) do
	--for _,nflws in pairs({1,10,100,1000,10000,20000,30000,40000,50000,60000,64000,65000,65535}) do
		--Heatup phase
		printf("heatup at %d rate for %d flows - %d secs", minRate, nflws, args.upheat);
		setRate(txDev:getTxQueue(0), args.size, minRate);
		local loadTask = mg.startTask("loadSlave", txDev:getTxQueue(0), rxDev, args.size, nflws, args.upheat)
		local snt, rcv = loadTask:wait()
		printf("heatup results: %d sent, %f loss", snt, (snt-rcv)/snt);
		if (rcv < snt/100) then
			printf("unsuccessful exiting");
			return	
		end
		mg.waitForTasks()
		local steps = 11;
		local upperbound = maxRate
		local lowerbound = minRate
		-- Testing phase
		for i = 1, steps  do
			printf("running step %d/%d for %d flows, %d Mbps", i, steps, nflws, rate);
			setRate(txDev:getTxQueue(0), args.size, rate);
			local packetsSent
			local packetsRecv
			local loadTask = mg.startTask("loadSlave", txDev:getTxQueue(0), rxDev, args.size, nflws, args.timeout)
			packetsSent, packetsRecv = loadTask:wait()
			local loss = (packetsSent - packetsRecv)/packetsSent
			printf("total: %d flows, %d rate, %d sent, %f lost",
				nflws, rate, packetsSent, loss);
			mg.waitForTasks()
			if (i+1 == steps) then
				file:write(nflws .. " " .. rate .. " " .. packetsSent .. " " ..
					   packetsSent/args.timeout .. " " .. loss .. "\n")
			end
			if (loss < 0.001) then
				lowerbound = rate
				rate = rate + (upperbound - rate)/2
			else
				upperbound = rate
				rate = lowerbound + (rate - lowerbound)/2
			end
		end
	end
end

local function fillUdpPacket(buf, len)
	buf:getUdpPacket():fill{
		ethSrc = queue,
		ethDst = DST_MAC,
		ip4Src = SRC_IP,
		ip4Dst = DST_IP,
		udpSrc = SRC_PORT,
		udpDst = DST_PORT,
		pktLength = len
	}
end

function loadSlave(queue, rxDev, size, flows, duration)
	local mempool = memory.createMemPool(function(buf)
		fillUdpPacket(buf, size)
	end)
	local bufs = mempool:bufArray()
	local counter = 0
	local finished = timer:new(duration)
	local fileTxCtr = stats:newDevTxCounter("txpkts", queue, "CSV", "txpkts.csv")
	local fileRxCtr = stats:newDevRxCounter("rxpkts", rxDev, "CSV", "rxpkts.csv")
	local txCtr = stats:newDevTxCounter(flows .. " tx", queue, "nil")
	local rxCtr = stats:newDevRxCounter(flows .. " rx", rxDev, "nil")
	local baseIP = parseIPAddress(SRC_IP_BASE)
	local baseSRCP = SRC_PORT
	local baseDSTP = DST_PORT
	while finished:running() and mg.running() do
		bufs:alloc(size)
		for i, buf in ipairs(bufs) do
			local pkt = buf:getUdpPacket()
			-- pkt.ip4.src:set(baseIP + counter)
			pkt.eth.src:set(SRC_MAC_BASE+counter)
			-- pkt.udp.src = (baseSRCP + counter)
			pkt.eth.dst:set(DST_MAC_BASE+counter)
			counter = incAndWrap(counter, flows)
		end
		-- UDP checksums are optional, so using just IPv4 checksums would be sufficient here
		bufs:offloadUdpChecksums()
		queue:send(bufs)
		txCtr:update()
		fileTxCtr:update()
		rxCtr:update()
		fileRxCtr:update()
	end
	txCtr:finalize()
	fileTxCtr:finalize()
	rxCtr:finalize()
	fileRxCtr:finalize()
	return txCtr.total, rxCtr.total
end

