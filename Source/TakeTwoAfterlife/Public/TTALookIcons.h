#pragma once

class UTexture2D;

// Hat icons are Art/UI/Looks/T_UI_Hat_<Name>.png. Faces are /Game/Characters/Faces/T_Face_<Name>.
UTexture2D* TTALoadHatIcon(const char* Name);
UTexture2D* TTALoadFaceTexture(const char* Name);
