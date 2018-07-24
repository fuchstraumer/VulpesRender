#pragma once
#ifndef VPR_ALLOC_DEBUG_VISUALIZATION_HPP
#define VPR_ALLOC_DEBUG_VISUALIZATION_HPP

namespace vpr {

    class Allocator;
    struct DebugVisualizationImpl;

    class DebugVisualization {
        DebugVisualization(const DebugVisualization&) = delete;
        DebugVisualization& operator=(const DebugVisualization&) = delete;
    public:

        DebugVisualization(Allocator* allocator);

        void Draw();
        void Update();
        
    private:
        DebugVisualizationImpl* impl;    
    };

}

#endif //!VPR_ALLOC_DEBUG_VISUALIZATION_HPP
