#                     DO NOT EDIT THIS FILE
# 
#  This file has been generated by the script core/avmfeatures.as,
#  from a set of configuration parameters in that file.
# 
#  If you feel you need to make changes below, instead edit the configuration
#  file and rerun it to get a new version of this file.
# 
#  ***** BEGIN LICENSE BLOCK *****
#  Version: MPL 1.1/GPL 2.0/LGPL 2.1
# 
#  The contents of this file are subject to the Mozilla Public License Version
#  1.1 (the "License"); you may not use this file except in compliance with
#  the License. You may obtain a copy of the License at
#  http://www.mozilla.org/MPL/
# 
#  Software distributed under the License is distributed on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
#  for the specific language governing rights and limitations under the
#  License.
# 
#  The Original Code is [Open Source Virtual Machine.].
# 
#  The Initial Developer of the Original Code is
#  Adobe System Incorporated.
#  Portions created by the Initial Developer are Copyright (C) 2009
#  the Initial Developer. All Rights Reserved.
# 
#  Contributor(s):
#    Adobe AS3 Team
# 
#  Alternatively, the contents of this file may be used under the terms of
#  either the GNU General Public License Version 2 or later (the "GPL"), or
#  the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
#  in which case the provisions of the GPL or the LGPL are applicable instead
#  of those above. If you wish to allow use of your version of this file only
#  under the terms of either the GPL or the LGPL, and not to allow others to
#  use your version of this file under the terms of the MPL, indicate your
#  decision by deleting the provisions above and replace them with the notice
#  and other provisions required by the GPL or the LGPL. If you do not delete
#  the provisions above, a recipient may use your version of this file under
#  the terms of any one of the MPL, the GPL or the LGPL.
# 
#  ***** END LICENSE BLOCK ****

def featureSettings(o):
    args = ""
    if o.getBoolArg("debugger"):
        args += "-DAVMFEATURE_DEBUGGER=1 "
    if o.getBoolArg("allocation-sampler"):
        args += "-DAVMFEATURE_ALLOCATION_SAMPLER=1 "
    if o.getBoolArg("vtune"):
        args += "-DAVMFEATURE_VTUNE=1 "
    if o.getBoolArg("jit"):
        args += "-DAVMFEATURE_JIT=1 "
    if o.getBoolArg("abc-interp"):
        args += "-DAVMFEATURE_ABC_INTERP=1 -DAVMFEATURE_WORDCODE_INTERP=0 "
    if o.getBoolArg("wordcode-interp"):
        args += "-DAVMFEATURE_WORDCODE_INTERP=1 -DAVMFEATURE_ABC_INTERP=0 "
    if o.getBoolArg("threaded-interp"):
        args += "-DAVMFEATURE_THREADED_INTERP=1 "
    if o.getBoolArg("selftest"):
        args += "-DAVMFEATURE_SELFTEST=1 "
    if o.getBoolArg("eval"):
        args += "-DAVMFEATURE_EVAL=1 "
    if o.getBoolArg("protect-jitmem"):
        args += "-DAVMFEATURE_PROTECT_JITMEM=1 "
    if o.getBoolArg("shared-gcheap"):
        args += "-DAVMFEATURE_SHARED_GCHEAP=1 "
    if o.getBoolArg("use-system_malloc"):
        args += "-DAVMFEATURE_USE_SYSTEM_MALLOC=1 "
    if o.getBoolArg("cpp-exceptions"):
        args += "-DAVMFEATURE_CPP_EXCEPTIONS=1 "
    if o.getBoolArg("interior-pointers"):
        args += "-DAVMFEATURE_INTERIOR_POINTERS=1 "
    if o.getBoolArg("jni"):
        args += "-DAVMFEATURE_JNI=1 "
    if o.getBoolArg("heap-alloca"):
        args += "-DAVMFEATURE_HEAP_ALLOCA=1 "
    if o.getBoolArg("static-function_ptrs"):
        args += "-DAVMFEATURE_STATIC_FUNCTION_PTRS=1 "
    return args
