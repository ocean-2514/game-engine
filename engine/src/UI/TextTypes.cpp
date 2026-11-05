#include "UI/TextTypes.h"


namespace eng {

bool TextStyle::operator==(const TextStyle& other) const {
    return pixelSize == other.pixelSize
        && color == other.color
        && align == other.align
        && verticalAlign == other.verticalAlign
        && wrap == other.wrap
        && lineSpacing == other.lineSpacing
        && pixelSnap == other.pixelSnap
        && tabSize == other.tabSize;
}


}