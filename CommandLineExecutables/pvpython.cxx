/*=========================================================================

Program:   ParaView
Module:    pvpython.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pvpython.h" // Include this first.
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#ifndef BUILD_SHARED_LIBS
#include "pvStaticPluginsInit.h"
#include "vtkPython.h"
#endif

extern "C" void initmultiarray(void);
extern "C" void initscalarmath(void);
extern "C" void initumath(void);
#ifdef HAVE_NUMPY_BLAS_LIB
extern "C" void init_dotblas(void);
#endif
extern "C" void init_sort(void);
extern "C" void initfftpack_lite(void);
extern "C" void init_compiled_base(void);
extern "C" void initlapack_lite(void);
extern "C" void init_capi(void);
extern "C" void initmtrand(void);

static void paraview_import_static_modules()
{
  PyImport_AppendInittab("numpy.core.multiarray", initmultiarray);
  PyImport_AppendInittab("numpy.core.scalarmath", initscalarmath);
  PyImport_AppendInittab("numpy.core.umath", initumath);
#ifdef HAVE_NUMPY_BLAS_LIB
  PyImport_AppendInittab("numpy.core._dotblas", init_dotblas);
#endif
  PyImport_AppendInittab("numpy.core._sort", init_sort);
  PyImport_AppendInittab("numpy.fft.fftpack_lite", initfftpack_lite);
  PyImport_AppendInittab("numpy.lib._compiled_base", init_compiled_base);
  PyImport_AppendInittab("numpy.linalg.lapack_lite", initlapack_lite);
  PyImport_AppendInittab("numpy.numarray._capi", init_capi);
  PyImport_AppendInittab("numpy.random.mtrand", initmtrand);
}

int main(int argc, char* argv[])
{
  paraview_import_static_modules();
#ifndef BUILD_SHARED_LIBS
  paraview_static_plugins_init();
#endif
  return ParaViewPython::Run(vtkProcessModule::PROCESS_CLIENT, argc, argv);
}
