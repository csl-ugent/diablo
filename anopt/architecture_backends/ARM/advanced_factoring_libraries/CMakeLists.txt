# only if building shared libraries
if(BUILD_SHARED_LIBS)
  add_library(afDefaultScore SHARED default_score.cc)
  target_link_libraries(afDefaultScore PUBLIC AnoptARM)

  add_library(afExponentialScore SHARED exponential_score.cc)
  target_link_libraries(afExponentialScore PUBLIC AnoptARM)

  add_library(afPartialScore SHARED partial_score.cc)
  target_link_libraries(afPartialScore PUBLIC AnoptARM)

  add_library(afDefaultCompare SHARED compare_default.cc)
  target_link_libraries(afDefaultCompare PUBLIC AnoptARM)

  add_library(afDecisionTreeCompare SHARED compare_decisiontree.cc)
  target_link_libraries(afDecisionTreeCompare PUBLIC AnoptARM)

  add_library(afRandomDispatcherChooser SHARED choose_dispatcher_random.cc)
  target_link_libraries(afRandomDispatcherChooser PUBLIC AnoptARM)
endif()
