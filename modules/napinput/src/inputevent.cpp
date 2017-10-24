#include <inputevent.h>

// RTTI Definitions
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::InputEvent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::KeyEvent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::KeyPressEvent)
	RTTI_CONSTRUCTOR(nap::EKeyCode, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::KeyReleaseEvent)
	RTTI_CONSTRUCTOR(nap::EKeyCode, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointerEvent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointerClickEvent)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointerPressEvent)
	RTTI_CONSTRUCTOR(int, int, nap::EMouseButton, int, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointerReleaseEvent)
	RTTI_CONSTRUCTOR(int, int, nap::EMouseButton, int, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointerDragEvent)
	RTTI_CONSTRUCTOR(int, int, nap::EMouseButton, int, int)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PointerMoveEvent)
	RTTI_CONSTRUCTOR(int, int, int, int, int, int)
RTTI_END_CLASS