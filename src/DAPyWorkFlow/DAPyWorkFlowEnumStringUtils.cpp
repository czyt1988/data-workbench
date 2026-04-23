#include "DAPyWorkFlowEnumStringUtils.h"

// ================================== DA::DAPyNodeState ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAPyNodeState,
                                  DA::Idle,
                                  { DA::Idle, "idle" },
                                  { DA::Waiting, "waiting" },
                                  { DA::Running, "running" },
                                  { DA::Success, "success" },
                                  { DA::Error, "error" },
                                  { DA::Skipped, "skipped" });