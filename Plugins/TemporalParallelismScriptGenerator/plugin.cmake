if (PARAVIEW_BUILD_QT_GUI AND PARAVIEW_ENABLE_PYTHON)
  pv_plugin(TemporalParallelismScriptGenerator
    DEFAULT_ENABLED
    DESCRIPTION "Plugin for creating Python spatio-temporal processing scripts")
endif()
