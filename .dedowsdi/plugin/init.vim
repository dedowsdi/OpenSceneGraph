set path+=include/osg

call misc#proj#load_map('c')

let g:external_files = ['/usr/local/source/OpenSceneGraph-Data']

command! -nargs=* RunFile exe printf(':CppMakeRun example_%s <args>', expand('%:t:r'))
