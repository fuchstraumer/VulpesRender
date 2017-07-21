#pragma once
#ifndef VULPES_VK_DEFERRED_H
#define VULPES_VK_DEFERRED_H

#include "stdafx.h"
#include "../ForwardDecl.h"
#include "Framebuffer.h"

namespace vulpes {

	class DeferredRenderer {
	public:

	private:

		OffscreenFramebuffer<hdr_framebuffer_t> framebuffer;
	};

}

#endif // !VULPES_VK_DEFERRED_H
