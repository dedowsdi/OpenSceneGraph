set wildignore+=*/utfcpp/*,utfcpp
set nospell
set path+=include/osg
let &viminfofile = getcwd() . '/.vim/.viminfo'
call misc#proj#load_map('c')
let g:mycpp_build_dir = './build/gcc/Debug'
let g:cdef_proj_name = 'osg'
let g:external_files = ['/usr/local/source/OpenSceneGraph-Data']
command! -nargs=* RunFile exe printf(':CppMakeRun example_%s <args>', expand('%:t:r'))
