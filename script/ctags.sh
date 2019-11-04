#!/bin/bash
cd "$(realpath "${BASH_SOURCE[0]%/*}")/.."
ctags -R --options="${HOME}/.ctags_options/cpp.ctags" include src
