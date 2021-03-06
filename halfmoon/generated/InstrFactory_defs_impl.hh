///
/// generated by templates.py -- do not edit
///

/// shape_reps[] gives the representations of
/// the shapes enumerated by InstrShape.
///
extern const ShapeRep shape_reps[SHAPE_MAX] = {
  { 0, 0, kVarIn },     // GOTOINSTR_SHAPE
  { 0, 0, kVarOut },    // ARMINSTR_SHAPE
  { 0, 0, kVarOut },    // CATCHBLOCKINSTR_SHAPE
  { 0, 0, kVarOut },    // LABELINSTR_SHAPE
  { 0, 1, kVarNone },   // CONSTANTEXPR_SHAPE
  { 0, 1, kVarOut },    // STARTINSTR_SHAPE
  { 1, 0, kVarIn },     // IFINSTR_SHAPE
  { 1, 0, kVarIn },     // SWITCHINSTR_SHAPE
  { 1, 1, kVarNone },   // UNARYEXPR_SHAPE
  { 2, 1, kVarNone },   // BINARYEXPR_SHAPE
  { 2, 1, kVarNone },   // SETLOCALINSTR_SHAPE
  { 1, 0, kVarIn },     // STOPINSTR_SHAPE
  { 1, 1, kVarIn },     // DEOPTSAFEPOINTINSTR_SHAPE
  { 1, 1, kVarNone },   // DEOPTFINISHINSTR_SHAPE
  { 1, 1, kVarNone },   // VOIDSTMT_SHAPE
  { 1, 2, kVarIn },     // NARYSTMT0_SHAPE
  { 1, 2, kVarIn },     // SAFEPOINTINSTR_SHAPE
  { 2, 1, kVarNone },   // DEBUGINSTR_SHAPE
  { 2, 1, kVarNone },   // DEOPTFINISHCALLINSTR_SHAPE
  { 2, 2, kVarIn },     // NARYSTMT1_SHAPE
  { 2, 2, kVarNone },   // UNARYSTMT_SHAPE
  { 3, 2, kVarIn },     // CALLSTMT2_SHAPE
  { 3, 2, kVarIn },     // NARYSTMT2_SHAPE
  { 3, 2, kVarNone },   // BINARYSTMT_SHAPE
  { 3, 4, kVarNone },   // HASNEXT2STMT_SHAPE
  { 4, 2, kVarIn },     // CALLSTMT3_SHAPE
  { 4, 2, kVarIn },     // NARYSTMT3_SHAPE
  { 5, 2, kVarIn },     // CALLSTMT4_SHAPE
  { 5, 2, kVarIn },     // NARYSTMT4_SHAPE
};

/// instr_attrs describes the instructions enumerated in InstrKind.
///
extern const InstrAttrs instr_attrs[HR_MAX] = {
  { "start",                 STARTINSTR_SHAPE, false },
  { "template",              STARTINSTR_SHAPE, false },
  { "catchblock",            CATCHBLOCKINSTR_SHAPE, false },
  { "return",                STOPINSTR_SHAPE, false },
  { "throw",                 STOPINSTR_SHAPE, false },
  { "goto",                  GOTOINSTR_SHAPE, false },
  { "label",                 LABELINSTR_SHAPE, false },
  { "if",                    IFINSTR_SHAPE, false },
  { "switch",                SWITCHINSTR_SHAPE, false },
  { "arm",                   ARMINSTR_SHAPE, false },
  { "const",                 CONSTANTEXPR_SHAPE, false },
  { "coerce",                BINARYSTMT_SHAPE, false },
  { "cast",                  BINARYSTMT_SHAPE, false },
  { "castobject",            UNARYEXPR_SHAPE, false },
  { "tonumber",              UNARYSTMT_SHAPE, false },
  { "toint",                 UNARYSTMT_SHAPE, false },
  { "touint",                UNARYSTMT_SHAPE, false },
  { "toboolean",             UNARYEXPR_SHAPE, false },
  { "d2b",                   UNARYEXPR_SHAPE, false },
  { "caststring",            UNARYSTMT_SHAPE, false },
  { "castns",                UNARYSTMT_SHAPE, false },
  { "cknull",                UNARYSTMT_SHAPE, false },
  { "cknullobject",          UNARYSTMT_SHAPE, false },
  { "cktimeout",             UNARYSTMT_SHAPE, false },
  { "abc_hasnext",           BINARYSTMT_SHAPE, false },
  { "abc_hasnext2",          HASNEXT2STMT_SHAPE, false },
  { "never",                 CONSTANTEXPR_SHAPE, false },
  { "call",                  CALLSTMT2_SHAPE, false },
  { "construct",             CALLSTMT2_SHAPE, false },
  { "constructsuper",        CALLSTMT2_SHAPE, false },
  { "loadenv_namespace",     BINARYEXPR_SHAPE, false },
  { "loadenv_boolean",       BINARYEXPR_SHAPE, false },
  { "loadenv_number",        BINARYEXPR_SHAPE, false },
  { "loadenv_string",        BINARYEXPR_SHAPE, false },
  { "loadenv_interface",     BINARYEXPR_SHAPE, false },
  { "loadenv",               BINARYEXPR_SHAPE, false },
  { "loadenv_atom",          BINARYEXPR_SHAPE, false },
  { "loadinitenv",           UNARYEXPR_SHAPE, false },
  { "loadsuperinitenv",      UNARYEXPR_SHAPE, false },
  { "loadenv_env",           BINARYEXPR_SHAPE, false },
  { "newobject",             NARYSTMT0_SHAPE, false },
  { "newarray",              NARYSTMT0_SHAPE, false },
  { "applytype",             NARYSTMT0_SHAPE, false },
  { "newinstance",           UNARYEXPR_SHAPE, false },
  { "abc_convert_s",         UNARYSTMT_SHAPE, false },
  { "abc_esc_xelem",         UNARYSTMT_SHAPE, false },
  { "abc_esc_xattr",         UNARYSTMT_SHAPE, false },
  { "abc_typeof",            UNARYEXPR_SHAPE, false },
  { "speculate_int",         BINARYEXPR_SHAPE, false },
  { "speculate_number",      BINARYEXPR_SHAPE, false },
  { "speculate_numeric",     BINARYEXPR_SHAPE, false },
  { "speculate_string",      BINARYEXPR_SHAPE, false },
  { "speculate_object",      BINARYEXPR_SHAPE, false },
  { "speculate_array",       BINARYEXPR_SHAPE, false },
  { "speculate_bool",        BINARYEXPR_SHAPE, false },
  { "ckfilter",              UNARYEXPR_SHAPE, false },
  { "abc_add",               BINARYSTMT_SHAPE, false },
  { "addd",                  BINARYEXPR_SHAPE, false },
  { "concat_strings",        BINARYEXPR_SHAPE, false },
  { "abc_nextname",          BINARYSTMT_SHAPE, false },
  { "abc_nextvalue",         BINARYSTMT_SHAPE, false },
  { "lessthan",              BINARYEXPR_SHAPE, false },
  { "lessequals",            BINARYEXPR_SHAPE, false },
  { "greaterthan",           BINARYEXPR_SHAPE, false },
  { "greaterequals",         BINARYEXPR_SHAPE, false },
  { "abc_instanceof",        BINARYSTMT_SHAPE, false },
  { "abc_istype",            BINARYSTMT_SHAPE, false },
  { "abc_istypelate",        BINARYSTMT_SHAPE, false },
  { "abc_astype",            BINARYEXPR_SHAPE, false },
  { "abc_astypelate",        BINARYSTMT_SHAPE, false },
  { "abc_in",                BINARYSTMT_SHAPE, false },
  { "divd",                  BINARYEXPR_SHAPE, false },
  { "modulo",                BINARYEXPR_SHAPE, false },
  { "subd",                  BINARYEXPR_SHAPE, false },
  { "muld",                  BINARYEXPR_SHAPE, false },
  { "addi",                  BINARYEXPR_SHAPE, false },
  { "subi",                  BINARYEXPR_SHAPE, false },
  { "muli",                  BINARYEXPR_SHAPE, false },
  { "ori",                   BINARYEXPR_SHAPE, false },
  { "andi",                  BINARYEXPR_SHAPE, false },
  { "xori",                  BINARYEXPR_SHAPE, false },
  { "lshi",                  BINARYEXPR_SHAPE, false },
  { "rshi",                  BINARYEXPR_SHAPE, false },
  { "rshui",                 BINARYEXPR_SHAPE, false },
  { "noti",                  UNARYEXPR_SHAPE, false },
  { "negi",                  UNARYEXPR_SHAPE, false },
  { "negd",                  UNARYEXPR_SHAPE, false },
  { "not",                   UNARYEXPR_SHAPE, false },
  { "newactivation",         UNARYSTMT_SHAPE, false },
  { "abc_finddef",           BINARYSTMT_SHAPE, false },
  { "abc_findpropstrict",    NARYSTMT3_SHAPE, false },
  { "abc_findpropstrictx",   NARYSTMT4_SHAPE, false },
  { "abc_findpropstrictns",  NARYSTMT4_SHAPE, false },
  { "abc_findpropstrictnsx", NARYSTMT4_SHAPE, false },
  { "abc_findproperty",      NARYSTMT3_SHAPE, false },
  { "abc_findpropertyx",     NARYSTMT4_SHAPE, false },
  { "abc_findpropertyns",    NARYSTMT4_SHAPE, false },
  { "abc_findpropertynsx",   NARYSTMT4_SHAPE, false },
  { "newclass",              NARYSTMT2_SHAPE, false },
  { "newfunction",           NARYSTMT1_SHAPE, false },
  { "abc_getsuper",          CALLSTMT2_SHAPE, false },
  { "abc_getsuperx",         CALLSTMT3_SHAPE, false },
  { "abc_getsuperns",        CALLSTMT3_SHAPE, false },
  { "abc_getsupernsx",       CALLSTMT4_SHAPE, false },
  { "abc_getdescendants",    CALLSTMT2_SHAPE, false },
  { "abc_getdescendantsx",   CALLSTMT3_SHAPE, false },
  { "abc_getdescendantsns",  CALLSTMT3_SHAPE, false },
  { "abc_getdescendantsnsx", CALLSTMT4_SHAPE, false },
  { "abc_callprop",          CALLSTMT2_SHAPE, false },
  { "abc_callpropx",         CALLSTMT3_SHAPE, false },
  { "abc_callpropns",        CALLSTMT3_SHAPE, false },
  { "abc_callpropnsx",       CALLSTMT4_SHAPE, false },
  { "abc_callproplex",       CALLSTMT2_SHAPE, false },
  { "abc_callproplexx",      CALLSTMT3_SHAPE, false },
  { "abc_callproplexns",     CALLSTMT3_SHAPE, false },
  { "abc_callproplexnsx",    CALLSTMT4_SHAPE, false },
  { "abc_constructprop",     CALLSTMT2_SHAPE, false },
  { "abc_constructpropx",    CALLSTMT3_SHAPE, false },
  { "abc_constructpropns",   CALLSTMT3_SHAPE, false },
  { "abc_constructpropnsx",  CALLSTMT4_SHAPE, false },
  { "abc_callsuper",         CALLSTMT2_SHAPE, false },
  { "abc_callsuperx",        CALLSTMT3_SHAPE, false },
  { "abc_callsuperns",       CALLSTMT3_SHAPE, false },
  { "abc_callsupernsx",      CALLSTMT4_SHAPE, false },
  { "callstatic",            CALLSTMT2_SHAPE, false },
  { "callmethod",            CALLSTMT2_SHAPE, false },
  { "callinterface",         CALLSTMT2_SHAPE, false },
  { "newcatch",              UNARYSTMT_SHAPE, false },
  { "setslot",               CALLSTMT2_SHAPE, false },
  { "getslot",               CALLSTMT2_SHAPE, false },
  { "slottype",              BINARYEXPR_SHAPE, false },
  { "getouterscope",         BINARYEXPR_SHAPE, false },
  { "safepoint",             SAFEPOINTINSTR_SHAPE, false },
  { "setlocal",              SETLOCALINSTR_SHAPE, false },
  { "newstate",              CONSTANTEXPR_SHAPE, false },
  { "deopt_safepoint",       DEOPTSAFEPOINTINSTR_SHAPE, false },
  { "deopt_finish",          DEOPTFINISHINSTR_SHAPE, false },
  { "deopt_finishcall",      DEOPTFINISHCALLINSTR_SHAPE, false },
  { "debugline",             DEBUGINSTR_SHAPE, false },
  { "debugfile",             DEBUGINSTR_SHAPE, false },
  { "string2atom",           UNARYEXPR_SHAPE, false },
  { "double2atom",           UNARYEXPR_SHAPE, false },
  { "int2atom",              UNARYEXPR_SHAPE, false },
  { "uint2atom",             UNARYEXPR_SHAPE, false },
  { "scriptobject2atom",     UNARYEXPR_SHAPE, false },
  { "bool2atom",             UNARYEXPR_SHAPE, false },
  { "ns2atom",               UNARYEXPR_SHAPE, false },
  { "atom2bool",             UNARYEXPR_SHAPE, false },
  { "atom2double",           UNARYEXPR_SHAPE, false },
  { "atom2string",           UNARYEXPR_SHAPE, false },
  { "atom2int",              UNARYEXPR_SHAPE, false },
  { "atom2uint",             UNARYEXPR_SHAPE, false },
  { "atom2scriptobject",     UNARYEXPR_SHAPE, false },
  { "atom2ns",               UNARYEXPR_SHAPE, false },
  { "i2d",                   UNARYEXPR_SHAPE, false },
  { "u2d",                   UNARYEXPR_SHAPE, false },
  { "d2i",                   UNARYEXPR_SHAPE, false },
  { "d2u",                   UNARYEXPR_SHAPE, false },
  { "toslot",                BINARYEXPR_SHAPE, false },
  { "toprimitive",           UNARYSTMT_SHAPE, false },
  { "eqi",                   BINARYEXPR_SHAPE, false },
  { "lti",                   BINARYEXPR_SHAPE, false },
  { "lei",                   BINARYEXPR_SHAPE, false },
  { "gti",                   BINARYEXPR_SHAPE, false },
  { "gei",                   BINARYEXPR_SHAPE, false },
  { "eqd",                   BINARYEXPR_SHAPE, false },
  { "ltd",                   BINARYEXPR_SHAPE, false },
  { "led",                   BINARYEXPR_SHAPE, false },
  { "gtd",                   BINARYEXPR_SHAPE, false },
  { "ged",                   BINARYEXPR_SHAPE, false },
  { "equi",                  BINARYEXPR_SHAPE, false },
  { "ltui",                  BINARYEXPR_SHAPE, false },
  { "leui",                  BINARYEXPR_SHAPE, false },
  { "gtui",                  BINARYEXPR_SHAPE, false },
  { "geui",                  BINARYEXPR_SHAPE, false },
  { "eqb",                   BINARYEXPR_SHAPE, false },
  { "doubletoint32",         UNARYEXPR_SHAPE, false },
  { "i2u",                   UNARYEXPR_SHAPE, false },
  { "u2i",                   UNARYEXPR_SHAPE, false },
  { "abc_dxnslate",          UNARYSTMT_SHAPE, false },
  { "abc_dxns",              UNARYSTMT_SHAPE, false },
  { "li8",                   UNARYSTMT_SHAPE, false },
  { "li16",                  UNARYSTMT_SHAPE, false },
  { "li32",                  UNARYSTMT_SHAPE, false },
  { "lf32",                  UNARYSTMT_SHAPE, false },
  { "lf64",                  UNARYSTMT_SHAPE, false },
  { "si8",                   BINARYSTMT_SHAPE, false },
  { "si16",                  BINARYSTMT_SHAPE, false },
  { "si32",                  BINARYSTMT_SHAPE, false },
  { "sf32",                  BINARYSTMT_SHAPE, false },
  { "sf64",                  BINARYSTMT_SHAPE, false },
  { "abc_equals",            BINARYSTMT_SHAPE, false },
  { "eqp",                   BINARYEXPR_SHAPE, false },
  { "eqs",                   BINARYEXPR_SHAPE, false },
  { "abc_strictequals",      BINARYEXPR_SHAPE, false },
  { "abc_setsuper",          CALLSTMT2_SHAPE, false },
  { "abc_setsuperx",         CALLSTMT3_SHAPE, false },
  { "abc_setsuperns",        CALLSTMT3_SHAPE, false },
  { "abc_setsupernsx",       CALLSTMT4_SHAPE, false },
  { "abc_deleteprop",        CALLSTMT2_SHAPE, false },
  { "abc_deletepropx",       CALLSTMT3_SHAPE, false },
  { "abc_deletepropns",      CALLSTMT3_SHAPE, false },
  { "abc_deletepropnsx",     CALLSTMT4_SHAPE, false },
  { "abc_getprop",           CALLSTMT2_SHAPE, false },
  { "abc_getpropx",          CALLSTMT3_SHAPE, false },
  { "getpropertylate_u",     BINARYSTMT_SHAPE, false },
  { "getpropertylate_i",     BINARYSTMT_SHAPE, false },
  { "getpropertylate_d",     BINARYSTMT_SHAPE, false },
  { "array_get_u",           BINARYSTMT_SHAPE, false },
  { "array_get_i",           BINARYSTMT_SHAPE, false },
  { "array_get_d",           BINARYSTMT_SHAPE, false },
  { "vectorint_get_u",       BINARYSTMT_SHAPE, false },
  { "vectorint_get_i",       BINARYSTMT_SHAPE, false },
  { "vectorint_get_d",       BINARYSTMT_SHAPE, false },
  { "vectoruint_get_u",      BINARYSTMT_SHAPE, false },
  { "vectoruint_get_i",      BINARYSTMT_SHAPE, false },
  { "vectoruint_get_d",      BINARYSTMT_SHAPE, false },
  { "vectordouble_get_u",    BINARYSTMT_SHAPE, false },
  { "vectordouble_get_i",    BINARYSTMT_SHAPE, false },
  { "vectordouble_get_d",    BINARYSTMT_SHAPE, false },
  { "abc_getpropns",         CALLSTMT3_SHAPE, false },
  { "abc_getpropnsx",        CALLSTMT4_SHAPE, false },
  { "abc_setprop",           CALLSTMT2_SHAPE, false },
  { "abc_setpropx",          CALLSTMT3_SHAPE, false },
  { "setpropertylate_u",     CALLSTMT2_SHAPE, false },
  { "setpropertylate_i",     CALLSTMT2_SHAPE, false },
  { "setpropertylate_d",     CALLSTMT2_SHAPE, false },
  { "array_set_u",           CALLSTMT2_SHAPE, false },
  { "array_set_i",           CALLSTMT2_SHAPE, false },
  { "array_set_d",           CALLSTMT2_SHAPE, false },
  { "vectorint_set_u",       CALLSTMT2_SHAPE, false },
  { "vectorint_set_i",       CALLSTMT2_SHAPE, false },
  { "vectorint_set_d",       CALLSTMT2_SHAPE, false },
  { "vectoruint_set_u",      CALLSTMT2_SHAPE, false },
  { "vectoruint_set_i",      CALLSTMT2_SHAPE, false },
  { "vectoruint_set_d",      CALLSTMT2_SHAPE, false },
  { "vectordouble_set_u",    CALLSTMT2_SHAPE, false },
  { "vectordouble_set_i",    CALLSTMT2_SHAPE, false },
  { "vectordouble_set_d",    CALLSTMT2_SHAPE, false },
  { "abc_setpropns",         CALLSTMT3_SHAPE, false },
  { "abc_setpropnsx",        CALLSTMT4_SHAPE, false },
  { "abc_initprop",          CALLSTMT2_SHAPE, false },
  { "abc_initpropx",         CALLSTMT3_SHAPE, false },
  { "abc_initpropns",        CALLSTMT3_SHAPE, false },
  { "abc_initpropnsx",       CALLSTMT4_SHAPE, false },
  { "abc_increment",         UNARYSTMT_SHAPE, true },
  { "abc_decrement",         UNARYSTMT_SHAPE, true },
  { "abc_increment_i",       UNARYSTMT_SHAPE, true },
  { "abc_decrement_i",       UNARYSTMT_SHAPE, true },
  { "abc_not",               UNARYSTMT_SHAPE, true },
  { "abc_negate",            UNARYSTMT_SHAPE, true },
  { "abc_bitnot",            UNARYSTMT_SHAPE, true },
  { "abc_negate_i",          UNARYSTMT_SHAPE, true },
  { "abc_sxi1",              UNARYSTMT_SHAPE, true },
  { "abc_sxi8",              UNARYSTMT_SHAPE, true },
  { "abc_sxi16",             UNARYSTMT_SHAPE, true },
  { "abc_checkfilter",       UNARYSTMT_SHAPE, true },
  { "abc_li8",               UNARYSTMT_SHAPE, true },
  { "abc_li16",              UNARYSTMT_SHAPE, true },
  { "abc_li32",              UNARYSTMT_SHAPE, true },
  { "abc_lf32",              UNARYSTMT_SHAPE, true },
  { "abc_lf64",              UNARYSTMT_SHAPE, true },
  { "abc_si8",               BINARYSTMT_SHAPE, true },
  { "abc_si16",              BINARYSTMT_SHAPE, true },
  { "abc_si32",              BINARYSTMT_SHAPE, true },
  { "abc_sf32",              BINARYSTMT_SHAPE, true },
  { "abc_sf64",              BINARYSTMT_SHAPE, true },
  { "abc_subtract",          BINARYSTMT_SHAPE, true },
  { "abc_multiply",          BINARYSTMT_SHAPE, true },
  { "abc_modulo",            BINARYSTMT_SHAPE, true },
  { "abc_divide",            BINARYSTMT_SHAPE, true },
  { "abc_add_i",             BINARYSTMT_SHAPE, true },
  { "abc_subtract_i",        BINARYSTMT_SHAPE, true },
  { "abc_multiply_i",        BINARYSTMT_SHAPE, true },
  { "abc_bitor",             BINARYSTMT_SHAPE, true },
  { "abc_bitand",            BINARYSTMT_SHAPE, true },
  { "abc_bitxor",            BINARYSTMT_SHAPE, true },
  { "abc_lshift",            BINARYSTMT_SHAPE, true },
  { "abc_rshift",            BINARYSTMT_SHAPE, true },
  { "abc_urshift",           BINARYSTMT_SHAPE, true },
  { "add_ui",                BINARYSTMT_SHAPE, true },
  { "add_nn",                BINARYSTMT_SHAPE, true },
  { "add_ss",                BINARYSTMT_SHAPE, true },
  { "abc_lessthan",          BINARYSTMT_SHAPE, true },
  { "abc_greaterequals",     BINARYSTMT_SHAPE, true },
  { "abc_greaterthan",       BINARYSTMT_SHAPE, true },
  { "abc_lessequals",        BINARYSTMT_SHAPE, true },
  { "abc_equals_ii",         BINARYSTMT_SHAPE, true },
  { "abc_equals_uu",         BINARYSTMT_SHAPE, true },
  { "abc_equals_bb",         BINARYSTMT_SHAPE, true },
  { "abc_equals_nn",         BINARYSTMT_SHAPE, true },
  { "abc_equals_pp",         BINARYSTMT_SHAPE, true },
  { "abc_equals_ss",         BINARYSTMT_SHAPE, true },
  { "abc_getslot",           CALLSTMT2_SHAPE, true },
  { "abc_getprop_slot",      CALLSTMT2_SHAPE, true },
  { "getpropx_u",            CALLSTMT3_SHAPE, true },
  { "getpropx_i",            CALLSTMT3_SHAPE, true },
  { "getpropx_d",            CALLSTMT3_SHAPE, true },
  { "getpropx_au",           CALLSTMT3_SHAPE, true },
  { "getpropx_ai",           CALLSTMT3_SHAPE, true },
  { "getpropx_ad",           CALLSTMT3_SHAPE, true },
  { "getpropx_viu",          CALLSTMT3_SHAPE, true },
  { "getpropx_vii",          CALLSTMT3_SHAPE, true },
  { "getpropx_vid",          CALLSTMT3_SHAPE, true },
  { "getpropx_vuu",          CALLSTMT3_SHAPE, true },
  { "getpropx_vui",          CALLSTMT3_SHAPE, true },
  { "getpropx_vud",          CALLSTMT3_SHAPE, true },
  { "getpropx_vdu",          CALLSTMT3_SHAPE, true },
  { "getpropx_vdi",          CALLSTMT3_SHAPE, true },
  { "getpropx_vdd",          CALLSTMT3_SHAPE, true },
  { "abc_setprop_slot",      CALLSTMT2_SHAPE, true },
  { "setpropx_u",            CALLSTMT3_SHAPE, true },
  { "setpropx_i",            CALLSTMT3_SHAPE, true },
  { "setpropx_d",            CALLSTMT3_SHAPE, true },
  { "setpropx_au",           CALLSTMT3_SHAPE, true },
  { "setpropx_ai",           CALLSTMT3_SHAPE, true },
  { "setpropx_ad",           CALLSTMT3_SHAPE, true },
  { "setpropx_viu",          CALLSTMT3_SHAPE, true },
  { "setpropx_vii",          CALLSTMT3_SHAPE, true },
  { "setpropx_vid",          CALLSTMT3_SHAPE, true },
  { "setpropx_vuu",          CALLSTMT3_SHAPE, true },
  { "setpropx_vui",          CALLSTMT3_SHAPE, true },
  { "setpropx_vud",          CALLSTMT3_SHAPE, true },
  { "setpropx_vdu",          CALLSTMT3_SHAPE, true },
  { "setpropx_vdi",          CALLSTMT3_SHAPE, true },
  { "setpropx_vdd",          CALLSTMT3_SHAPE, true },
  { "abc_setslot",           CALLSTMT2_SHAPE, true },
  { "findprop2finddef",      NARYSTMT1_SHAPE, true },
  { "findprop2getouter",     NARYSTMT1_SHAPE, true },
  { "callprop_string",       CALLSTMT2_SHAPE, true },
  { "coerce_any",            BINARYSTMT_SHAPE, true },
  { "coerce_object",         BINARYSTMT_SHAPE, true },
  { "coerce_number",         BINARYSTMT_SHAPE, true },
  { "coerce_int",            BINARYSTMT_SHAPE, true },
  { "coerce_uint",           BINARYSTMT_SHAPE, true },
  { "coerce_boolean",        BINARYSTMT_SHAPE, true },
  { "coerce_string",         BINARYSTMT_SHAPE, true },
  { "coerce_ns",             BINARYSTMT_SHAPE, true },
  { "inttouint",             UNARYSTMT_SHAPE, true },
  { "uinttoint",             UNARYSTMT_SHAPE, true },
  { "numbertoint",           UNARYSTMT_SHAPE, true },
  { "numbertouint",          UNARYSTMT_SHAPE, true },
  { "abc_modulo_and",        BINARYSTMT_SHAPE, true },
};

