// DAAgentAPI.h
#pragma once
#include <QtCore/QtGlobal>

#if defined(DAAGENT_BUILD)
#define DAAgent_API Q_DECL_EXPORT
#else
#define DAAgent_API Q_DECL_IMPORT
#endif
