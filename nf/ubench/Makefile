
TARGETDIR=./build
run: map-ubench
	@./map_ubench

.FORCE:

worst-some-collisions: worst-some-collisions.c .FORCE
	@gcc worst-some-collisions.c -O2 -I .. flow-no-hash.c \
	    -D NUM_COLLISIONS=${NUM_COLLISIONS} \
	    ../lib/flowmanager.c ../lib/flowtable.c \
	    ../lib/expirator.c \
	    ../lib/containers/double-chain.c ../lib/containers/double-chain-impl.c \
	    ../lib/containers/map.c \
	    ../lib/containers/double-map.c ../lib/containers/map-impl.c \
	    ../lib/containers/vector.c \
	    dummy-log.c \
	    -o $(TARGETDIR)/worst-some-collisions

worst-no-collisions: worst-no-collisions.c .FORCE
	@gcc worst-no-collisions.c -O2 -I .. flow-no-hash.c \
	    ../lib/flowmanager.c ../lib/flowtable.c \
	    ../lib/expirator.c \
	    ../lib/containers/double-chain.c ../lib/containers/double-chain-impl.c \
	    ../lib/containers/map.c \
	    ../lib/containers/double-map.c ../lib/containers/map-impl.c \
	    ../lib/containers/vector.c \
	    dummy-log.c \
	    -o $(TARGETDIR)/worst-no-collisions

cache-limitation-ubench: map_test.c .FORCE
	@gcc map_test.c -O2 -I .. flow-no-hash.c \
	    ../lib/containers/map.c ../lib/containers/map-impl.c \
            dummy-log.c \
	   -DUSE_AND_FOR_HASH -DREAL  -o map_ubench_real
	@gcc map_test.c -O2 -I .. flow-no-hash.c \
            ../lib/containers/map.c ../lib/containers/map-impl.c \
            dummy-log.c \
           -DUSE_AND_FOR_HASH -DCLEARED_CACHE  -o map_ubench_clear_cache
	@gcc map_test.c -O2 -I .. flow-no-hash.c \
            ../lib/containers/map.c ../lib/containers/map-impl.c \
            dummy-log.c \
           -DUSE_AND_FOR_HASH -DARTIFICIAL  -o $(TARGETDIR)/map_ubench_artificial

expirator-ubench: expirator_test.c .FORCE
	@gcc expirator_test.c -O2 -I .. flow-no-hash.c \
            ../lib/flowmanager.c ../lib/flowtable.c \
            ../lib/expirator.c \
            ../lib/containers/double-chain.c ../lib/containers/double-chain-impl.c \
            ../lib/containers/map.c \
            ../lib/containers/double-map.c ../lib/containers/map-impl.c \
            ../lib/containers/vector.c \
	    dummy-log.c -DUSE_AND_FOR_HASH \
	    -o $(TARGETDIR)/expirator_ubench  

libVig-ubench: libVig_ubench.c .FORCE
	@gcc libVig_ubench.c -g -O2 -fno-stack-protector -z execstack -I .. ../lib/flow.c \
            ../lib/flowmanager.c ../lib/flowtable.c \
            ../lib/expirator.c \
            ../lib/containers/double-chain.c ../lib/containers/double-chain-impl.c \
            ../lib/containers/map.c \
            ../lib/containers/double-map.c ../lib/containers/map-impl.c \
            ../lib/containers/vector.c \
            dummy-log.c -DUSE_AND_FOR_HASH \
            -o $(TARGETDIR)/libVig_ubench         
