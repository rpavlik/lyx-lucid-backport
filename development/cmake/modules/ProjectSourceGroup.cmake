#
#  Copyright (c) 2006, Peter K�mmel, <syntheticpp@gmx.net>
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  
#  1. Redistributions of source code must retain the copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products 
#     derived from this software without specific prior written permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
#  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
#  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
#  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
#  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  

# folders in the msvc projects
# mode==flat  : headers and ourses in no folders
# mode==split : standard behavior of cmake, split headers and sources
# mode== <other values" : code is in this folder

macro(project_source_group mode sources headers)
	#message(STATUS ${mode})
	#message(STATUS ${sources} ${headers})
	if(${mode} MATCHES "flat")
		source_group("Source Files" Files)
		source_group("Header Files" Files)
		source_group("cmake" FILES CMakeLists.txt)
	else(${mode} MATCHES "flat")
		if(NOT ${mode} MATCHES "split")
			source_group("${mode}" FILES ${${sources}} ${${headers}})
		endif(NOT ${mode} MATCHES "split")
	endif(${mode} MATCHES "flat")
endmacro(project_source_group mode sources headers)

