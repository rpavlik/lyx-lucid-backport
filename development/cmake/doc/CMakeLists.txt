# This file is part of LyX, the document processor.
# Licence details can be found in the file COPYING.
#
# Copyright (c) 2008-2011 Peter Kümmel, <syntheticpp@gmx.net>
# Copyright (c) 2008-2011 Kornel Benko, <Kornel.Benko@berlin.de>
#

project(doc)

SET(_docs)
file(GLOB_RECURSE _rel_lyx_docs RELATIVE "${TOP_SRC_DIR}/lib/doc" "${TOP_SRC_DIR}/lib/doc/*.lyx" "${TOP_SRC_DIR}/lib/doc/*.txt")

foreach(_rel_doc ${_rel_lyx_docs})
  if ("${_rel_doc}" MATCHES "/" )
    string(REGEX REPLACE "/[^/]*$" "" _rel_dir_part ${_rel_doc})
  else("${_rel_doc}" MATCHES "/")
    set(_rel_dir_part ".")
  endif("${_rel_doc}" MATCHES "/")
  set(_created_doc "${CMAKE_CURRENT_BINARY_DIR}/${_rel_doc}")
  get_filename_component(_doc_dir ${_created_doc} PATH)
  file(MAKE_DIRECTORY ${_doc_dir})
  #message("found relative file " ${_rel_doc})
  #message("input ${TOP_SRC_DIR}/lib/doc/${_rel_doc}")
  #message("output ${_created_doc}")
  #message("rel_dir_part ${_rel_dir_part}")
  SET_SOURCE_FILES_PROPERTIES(${_created_doc} GENERATED)
  add_custom_command(
    OUTPUT "${_created_doc}"
    COMMAND ${LYX_PYTHON_EXECUTABLE} "${TOP_SRC_DIR}/development/cmake/doc/ReplaceValues.py" "LYX_USERDIR_VER=${LYX_USERDIR_VER}" "LYX_DIR_VER=${LYX_DIR_VER}" "${TOP_SRC_DIR}/lib/doc/${_rel_doc}" > "${_created_doc}"
    DEPENDS "${TOP_SRC_DIR}/lib/doc/${_rel_doc}" "${TOP_SRC_DIR}/development/cmake/doc/ReplaceValues.py"
    )
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_rel_doc}" DESTINATION "${LYX_DATA_SUBDIR}doc/${_rel_dir_part}")
  LIST(APPEND _docs "${_created_doc}")
endforeach(_rel_doc)

ADD_CUSTOM_TARGET(doc ALL DEPENDS ${_docs})

