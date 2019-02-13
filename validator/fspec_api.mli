type lemma_params = {
  ret_name : string;
  ret_val : string;
  args : string list;
  arg_exps : Ir.tterm list;
  tmp_gen : string -> string;
  is_tip : bool;
  arg_types : Ir.ttype list;
  exptr_types : Ir.ttype Core.Std.String.Map.t;
  ret_type : Ir.ttype;
}
type blemma_params = {
  args : string list;
  arg_exps : Ir.tterm list;
  tmp_gen : string -> string;
  is_tip : bool;
  arg_types : Ir.ttype list;
  exptr_types : Ir.ttype Core.Std.String.Map.t;
  ret_type : Ir.ttype;
}
type lemma = lemma_params -> string
type blemma = blemma_params -> string
val tx_l : string -> 'a -> string
val tx_bl : string -> 'a -> string
val on_rez_nonzero : string -> lemma_params -> string
val on_rez_nz : (lemma_params -> string) -> lemma_params -> string
val render_deep_assignment : Ir.eq_condition -> string
val deep_copy : Ir.var_spec -> string
type type_set = Static of Ir.ttype | Dynamic of (string * Ir.ttype) list
val stt : Ir.ttype list -> type_set list
val estt : ('a * Ir.ttype) list -> ('a * type_set) list
type fun_spec = {
  ret_type : type_set;
  arg_types : type_set list;
  extra_ptr_types : (string * type_set) list;
  lemmas_before : blemma list;
  lemmas_after : lemma list;
}
module type Spec =
  sig
    val preamble : string
    val fun_types : fun_spec Core.Std.String.Map.t
    val fixpoints : Ir.tterm Core.Std.String.Map.t
    val boundary_fun : string
    val finishing_fun : string
    val eventproc_iteration_begin : string
    val eventproc_iteration_end : string
    val user_check_for_complete_iteration : string
  end
val spec : (module Spec) option ref
