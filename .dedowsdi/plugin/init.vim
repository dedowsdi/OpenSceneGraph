set path+=include/osg

call misc#proj#load_map('c')

let g:external_files = ['/usr/local/source/OpenSceneGraph-Data']
let g:cdef_get_set_style='camel'

command! -nargs=* RunFile exe printf(':CppMakeRun example_%s <args>', expand('%:t:r'))
