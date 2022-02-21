#
# PART B. DOWNLOADING AGREEMENT - LICENSE FROM SBIA WITH RIGHT TO SUBLICENSE ("SOFTWARE LICENSE").
#  ------------------------------------------------------------------------------------------------
#
#  1. As used in this Software License, "you" means the individual downloading and/or
#     using, reproducing, modifying, displaying and/or distributing the Software and
#     the institution or entity which employs or is otherwise affiliated with such
#     individual in connection therewith. The Section of Biomedical Image Analysis,
#     Department of Radiology at the Universiy of Pennsylvania ("SBIA") hereby grants
#     you, with right to sublicense, with respect to SBIA's rights in the software,
#     and data, if any, which is the subject of this Software License (collectively,
#     the "Software"), a royalty-free, non-exclusive license to use, reproduce, make
#     derivative works of, display and distribute the Software, provided that:
#     (a) you accept and adhere to all of the terms and conditions of this Software
#     License; (b) in connection with any copy of or sublicense of all or any portion
#     of the Software, all of the terms and conditions in this Software License shall
#     appear in and shall apply to such copy and such sublicense, including without
#     limitation all source and executable forms and on any user documentation,
#     prefaced with the following words: "All or portions of this licensed product
#     (such portions are the "Software") have been obtained under license from the
#     Section of Biomedical Image Analysis, Department of Radiology at the University
#     of Pennsylvania and are subject to the following terms and conditions:"
#     (c) you preserve and maintain all applicable attributions, copyright notices
#     and licenses included in or applicable to the Software; (d) modified versions
#     of the Software must be clearly identified and marked as such, and must not
#     be misrepresented as being the original Software; and (e) you consider making,
#     but are under no obligation to make, the source code of any of your modifications
#     to the Software freely available to others on an open source basis.
#
#  2. The license granted in this Software License includes without limitation the
#     right to (i) incorporate the Software into proprietary programs (subject to
#     any restrictions applicable to such programs), (ii) add your own copyright
#     statement to your modifications of the Software, and (iii) provide additional
#     or different license terms and conditions in your sublicenses of modifications
#     of the Software; provided that in each case your use, reproduction or
#     distribution of such modifications otherwise complies with the conditions
#     stated in this Software License.
#
#  3. This Software License does not grant any rights with respect to third party
#     software, except those rights that SBIA has been authorized by a third
#     party to grant to you, and accordingly you are solely responsible for
#     (i) obtaining any permissions from third parties that you need to use,
#     reproduce, make derivative works of, display and distribute the Software,
#     and (ii) informing your sublicensees, including without limitation your
#     end-users, of their obligations to secure any such required permissions.
#
#  4. The Software has been designed for research purposes only and has not been
#     reviewed or approved by the Food and Drug Administration or by any other
#     agency. YOU ACKNOWLEDGE AND AGREE THAT CLINICAL APPLICATIONS ARE NEITHER
#     RECOMMENDED NOR ADVISED. Any commercialization of the Software is at the
#     sole risk of the party or parties engaged in such commercialization.
#     You further agree to use, reproduce, make derivative works of, display
#     and distribute the Software in compliance with all applicable governmental
#     laws, regulations and orders, including without limitation those relating
#     to export and import control.
#
#  5. The Software is provided "AS IS" and neither SBIA nor any contributor to
#     the software (each a "Contributor") shall have any obligation to provide
#     maintenance, support, updates, enhancements or modifications thereto.
#     SBIA AND ALL CONTRIBUTORS SPECIFICALLY DISCLAIM ALL EXPRESS AND IMPLIED
#     WARRANTIES OF ANY KIND INCLUDING, BUT NOT LIMITED TO, ANY WARRANTIES OF
#     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
#     IN NO EVENT SHALL SBIA OR ANY CONTRIBUTOR BE LIABLE TO ANY PARTY FOR
#     DIRECT, INDIRECT, SPECIAL, INCIDENTAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES
#     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY ARISING IN ANY WAY RELATED
#     TO THE SOFTWARE, EVEN IF SBIA OR ANY CONTRIBUTOR HAS BEEN ADVISED OF THE
#     POSSIBILITY OF SUCH DAMAGES. TO THE MAXIMUM EXTENT NOT PROHIBITED BY LAW OR
#     REGULATION, YOU FURTHER ASSUME ALL LIABILITY FOR YOUR USE, REPRODUCTION,
#     MAKING OF DERIVATIVE WORKS, DISPLAY, LICENSE OR DISTRIBUTION OF THE SOFTWARE
#     AND AGREE TO INDEMNIFY AND HOLD HARMLESS SBIA AND ALL CONTRIBUTORS FROM
#     AND AGAINST ANY AND ALL CLAIMS, SUITS, ACTIONS, DEMANDS AND JUDGMENTS ARISING
#     THEREFROM.
#
#  6. None of the names, logos or trademarks of SBIA or any of SBIA's affiliates
#     or any of the Contributors, or any funding agency, may be used to endorse
#     or promote products produced in whole or in part by operation of the Software
#     or derived from or based on the Software without specific prior written
#     permission from the applicable party.
#
#  7. Any use, reproduction or distribution of the Software which is not in accordance
#     with this Software License shall automatically revoke all rights granted to you
#     under this Software License and render Paragraphs 1 and 2 of this Software
#     License null and void.
#
#  8. This Software License does not grant any rights in or to any intellectual
#     property owned by SBIA or any Contributor except those rights expressly
#     granted hereunder.
#
#
#  PART C. MISCELLANEOUS
#  ---------------------
#
#  This Agreement shall be governed by and construed in accordance with the laws
#  of The Commonwealth of Pennsylvania without regard to principles of conflicts
#  of law. This Agreement shall supercede and replace any license terms that you
#  may have agreed to previously with respect to Software from SBIA.
#
##############################################################################
# @file  FindSphinx.cmake
# @brief Find Sphinx documentation build tools.
#
# @par Input variables:
# <table border="0">
#   <tr>
#     @tp @b Sphinx_DIR @endtp
#     <td>Installation directory of Sphinx tools. Can also be set as environment variable.</td>
#   </tr>
#   <tr>
#     @tp @b SPHINX_DIR @endtp
#     <td>Alternative environment variable for @c Sphinx_DIR.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_FIND_COMPONENTS @endtp
#     <td>Sphinx build tools to look for, i.e., 'apidoc' and/or 'build'.</td>
#   </tr>
# </table>
#
# @par Output variables:
# <table border="0">
#   <tr>
#     @tp @b Sphinx_FOUND @endtp
#     <td>Whether all or only the requested Sphinx build tools were found.</td>
#   </tr>
#   <tr>
#     @tp @b SPHINX_FOUND @endtp
#     <td>Alias for @c Sphinx_FOUND.<td>
#   </tr>
#   <tr>
#     @tp @b SPHINX_EXECUTABLE @endtp
#     <td>Non-cached alias for @c Sphinx-build_EXECUTABLE.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_PYTHON_EXECUTABLE @endtp
#     <td>Python executable used to run sphinx-build. This is either the
#         by default found Python interpreter or a specific version as
#         specified by the shebang (#!) of the sphinx-build script.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_PYTHON_OPTIONS @endtp
#     <td>A list of Python options extracted from the shebang (#!) of the
#         sphinx-build script. The -E option is added by this module
#         if the Python executable is not the system default to avoid
#         problems with a differing setting of the @c PYTHONHOME.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx-build_EXECUTABLE @endtp
#     <td>Absolute path of the found sphinx-build tool.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx-apidoc_EXECUTABLE @endtp
#     <td>Absolute path of the found sphinx-apidoc tool.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_VERSION_STRING @endtp
#     <td>Sphinx version found e.g. 1.1.2.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_VERSION_MAJOR @endtp
#     <td>Sphinx major version found e.g. 1.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_VERSION_MINOR @endtp
#     <td>Sphinx minor version found e.g. 1.</td>
#   </tr>
#   <tr>
#     @tp @b Sphinx_VERSION_PATCH @endtp
#     <td>Sphinx patch version found e.g. 2.</td>
#   </tr>
# </table>
#
# @ingroup CMakeFindModules
##############################################################################

set (_Sphinx_REQUIRED_VARS)

# ----------------------------------------------------------------------------
# initialize search
if (NOT Sphinx_DIR)
  if (NOT $ENV{Sphinx_DIR} STREQUAL "")
    set (Sphinx_DIR "$ENV{Sphinx_DIR}" CACHE PATH "Installation prefix of Sphinx (docutils)." FORCE)
  else ()
    set (Sphinx_DIR "$ENV{SPHINX_DIR}" CACHE PATH "Installation prefix of Sphinx (docutils)." FORCE)
  endif ()
endif ()

# ----------------------------------------------------------------------------
# default components to look for
if (NOT Sphinx_FIND_COMPONENTS)
  set (Sphinx_FIND_COMPONENTS "build")
elseif (NOT Sphinx_FIND_COMPONENTS MATCHES "^(build|apidoc)$")
  message (FATAL_ERROR "Invalid Sphinx component in: ${Sphinx_FIND_COMPONENTS}")
endif ()

# ----------------------------------------------------------------------------
# find components, i.e., build tools
foreach (_Sphinx_TOOL IN LISTS Sphinx_FIND_COMPONENTS)
  if (Sphinx_DIR)
    find_program (
      Sphinx-${_Sphinx_TOOL}_EXECUTABLE
      NAMES         sphinx-${_Sphinx_TOOL} sphinx-${_Sphinx_TOOL}.py
      HINTS         "${Sphinx_DIR}"
      PATH_SUFFIXES bin
      DOC           "The sphinx-${_Sphinx_TOOL} Python script."
      NO_DEFAULT_PATH
    )
  else ()
    find_program (
      Sphinx-${_Sphinx_TOOL}_EXECUTABLE
      NAMES sphinx-${_Sphinx_TOOL} sphinx-${_Sphinx_TOOL}.py
      DOC   "The sphinx-${_Sphinx_TOOL} Python script."
    )
  endif ()
  mark_as_advanced (Sphinx-${_Sphinx_TOOL}_EXECUTABLE)
  list (APPEND _Sphinx_REQUIRED_VARS Sphinx-${_Sphinx_TOOL}_EXECUTABLE)
endforeach ()

# ----------------------------------------------------------------------------
# determine Python executable used by Sphinx
if (Sphinx-build_EXECUTABLE)
  # extract python executable from shebang of sphinx-build
  find_package (PythonInterp QUIET)
  set (Sphinx_PYTHON_EXECUTABLE "${PYTHON_EXECUTABLE}")
  set (Sphinx_PYTHON_OPTIONS)
  file (STRINGS "${Sphinx-build_EXECUTABLE}" FIRST_LINE LIMIT_COUNT 1)
  if (FIRST_LINE MATCHES "^#!(.*/python.*)") # does not match "#!/usr/bin/env python" !
    string (REGEX REPLACE "^ +| +$" "" Sphinx_PYTHON_EXECUTABLE "${CMAKE_MATCH_1}")
    if (Sphinx_PYTHON_EXECUTABLE MATCHES "([^ ]+) (.*)")
      set (Sphinx_PYTHON_EXECUTABLE "${CMAKE_MATCH_1}")
      string (REGEX REPLACE " +" ";" Sphinx_PYTHON_OPTIONS "${CMAKE_MATCH_2}")
    endif ()
  endif ()
  # this is done to avoid problems with multiple Python versions being installed
  # remember: CMake command if(STR EQUAL STR) is bad and may cause many troubles !
  string (REGEX REPLACE "([.+*?^$])" "\\\\\\1" _Sphinx_PYTHON_EXECUTABLE_RE "${PYTHON_EXECUTABLE}")
  list (FIND Sphinx_PYTHON_OPTIONS -E IDX)
  if (IDX EQUAL -1 AND NOT Sphinx_PYTHON_EXECUTABLE MATCHES "^${_Sphinx_PYTHON_EXECUTABLE_RE}$")
    list (INSERT Sphinx_PYTHON_OPTIONS 0 -E)
  endif ()
  unset (_Sphinx_PYTHON_EXECUTABLE_RE)
endif ()

# ----------------------------------------------------------------------------
# determine Sphinx version
if (Sphinx-build_EXECUTABLE)
  # intentionally use invalid -h option here as the help that is shown then
  # will include the Sphinx version information
  if (Sphinx_PYTHON_EXECUTABLE)
    execute_process (
      COMMAND "${Sphinx_PYTHON_EXECUTABLE}" ${Sphinx_PYTHON_OPTIONS} "${Sphinx-build_EXECUTABLE}" -h
      OUTPUT_VARIABLE _Sphinx_VERSION
      ERROR_VARIABLE  _Sphinx_VERSION
    )
  elseif (UNIX)
    execute_process (
      COMMAND "${Sphinx-build_EXECUTABLE}" -h
      OUTPUT_VARIABLE _Sphinx_VERSION
      ERROR_VARIABLE  _Sphinx_VERSION
    )
  endif ()

  # The sphinx version can also contain a "b" instead of the last dot.
  # For example "Sphinx v1.2b1" so we cannot just split on "."
  if (_Sphinx_VERSION MATCHES "Sphinx v([0-9]+\\.[0-9]+(\\.|b)[0-9]+)")
    set (Sphinx_VERSION_STRING "${CMAKE_MATCH_1}")
    string(REGEX REPLACE "([0-9]+)\\.[0-9]+(\\.|b)[0-9]+" "\\1" Sphinx_VERSION_MAJOR ${Sphinx_VERSION_STRING})
    string(REGEX REPLACE "[0-9]+\\.([0-9]+)(\\.|b)[0-9]+" "\\1" Sphinx_VERSION_MINOR ${Sphinx_VERSION_STRING})
    string(REGEX REPLACE "[0-9]+\\.[0-9]+(\\.|b)([0-9]+)" "\\1" Sphinx_VERSION_PATCH ${Sphinx_VERSION_STRING})

    # v1.2.0 -> v1.2
    if (Sphinx_VERSION_PATCH EQUAL 0)
      string (REGEX REPLACE "\\.0$" "" Sphinx_VERSION_STRING "${Sphinx_VERSION_STRING}")
    endif ()
  endif()
endif ()

# ----------------------------------------------------------------------------
# compatibility with FindPythonInterp.cmake and FindPerl.cmake
set (SPHINX_EXECUTABLE "${Sphinx-build_EXECUTABLE}")

# ----------------------------------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set SPHINX_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (
  Sphinx
  REQUIRED_VARS
    ${_Sphinx_REQUIRED_VARS}
#  VERSION_VAR # This isn't available until CMake 2.8.8 so don't use it.
    Sphinx_VERSION_STRING
)

# ----------------------------------------------------------------------------
# set Sphinx_DIR
if (NOT Sphinx_DIR AND Sphinx-build_EXECUTABLE)
  get_filename_component (Sphinx_DIR "${Sphinx-build_EXECUTABLE}" PATH)
  string (REGEX REPLACE "/bin/?" "" Sphinx_DIR "${Sphinx_DIR}")
  set (Sphinx_DIR "${Sphinx_DIR}" CACHE PATH "Installation directory of Sphinx tools." FORCE)
endif ()

unset (_Sphinx_VERSION)
unset (_Sphinx_REQUIRED_VARS)