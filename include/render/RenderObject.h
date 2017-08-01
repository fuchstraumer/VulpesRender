#ifndef VULPES_RENDER_OBJECT_H
#define VULPES_RENDER_OBJECT_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.h"

namespace vulpes {

	class RenderObject {
		RenderObject(const RenderObject&) = delete;
		RenderObject& operator=(const RenderObject&) = delete;
	public:



	private:

		virtual void createDescriptors();

	};

}

#endif // !VULPES_RENDER_OBJECT_H
