#pragma once
#ifndef COMMON_DEFINES_H
#define COMMON_DEFINES_H

namespace vulpes {

	enum class NodeStatus {
		Undefined, // Initial value for all nodes. If a node has this, it has not been properly constructed (or has been deleted)
		Active, // Being used in next frame
		Subdivided, // Has been subdivided, render children instead of this.
		NeedsUnload, // Erase and unload resources.
		NeedsTransfer,
		RequestData, // Request height data from the upsampling DataProducer
	};

	
}


#endif // !COMMON_DEFINES_H
