#include "DAWorkFlowEnumStringUtils.h"

// ================================== DA::DAAbstractNodeGraphicsItem::LinkPointLocation ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAAbstractNodeGraphicsItem::LinkPointLocation,
                                  DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide,
                                  { DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnLeftSide, "left-side" },
                                  { DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnTopSide, "top-side" },
                                  { DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnRightSide, "right-side" },
                                  { DA::DAAbstractNodeGraphicsItem::LinkPointLocationOnBottomSide, "bottom-side" });

// ================================== DA::DANodeLinkPoint::Way ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DANodeLinkPoint::Way,
                                  DA::DANodeLinkPoint::Input,
                                  { DA::DANodeLinkPoint::Input, "input" },
                                  { DA::DANodeLinkPoint::Output, "output" });
