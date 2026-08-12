build_lib_example(
  NAME ev-plc-transition-admission
  SOURCE_FILES ev-plc-transition-admission.cc
  LIBRARIES_TO_LINK ${libev-plc-transition}
)

build_lib_example(
  NAME trackc-e1-parity
  SOURCE_FILES trackc-e1-parity.cc
  LIBRARIES_TO_LINK ${libev-plc-transition}
)

build_lib_example(
  NAME trackc-e4-csma
  SOURCE_FILES trackc-e4-csma.cc
  LIBRARIES_TO_LINK ${libev-plc-transition}
)

build_lib_example(
  NAME trackc-e4-scheduled
  SOURCE_FILES trackc-e4-scheduled.cc
  LIBRARIES_TO_LINK ${libev-plc-transition}
)

build_lib_example(
  NAME trackc-g3
  SOURCE_FILES trackc-g3.cc
  LIBRARIES_TO_LINK ${libev-plc-transition}
)

build_lib_example(
  NAME trackc-thm1
  SOURCE_FILES trackc-thm1.cc
  LIBRARIES_TO_LINK ${libev-plc-transition}
)
