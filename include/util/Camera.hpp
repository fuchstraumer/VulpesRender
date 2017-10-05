#pragma once
#ifndef VULPES_CAMERA_H
#define VULPES_CAMERA_H
#include "vpr_stdafx.h"
#include "util/Ray.hpp"
#include "util/ViewFrustum.hpp"
#include "glm/gtc/quaternion.hpp"

namespace vulpes {

	// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
	enum class Direction : uint8_t {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN,
	};

    class cameraController;


	class cameraBase {
	public:

        virtual ~cameraBase() = default;

        //virtual void ProcessKeyboard(const Direction& dir, const float& delta_time);
		virtual void MouseDrag(const int& button, const float& x_offset, const float& y_offset) = 0;
		virtual void MouseScroll(const int& button, const float& y_scroll) = 0;
		virtual void MouseDown(const int& button, const float& x, const float& y) = 0;
		virtual void MouseUp(const int& button, const float& x, const float& y)= 0;

        glm::vec3 GetEyeLocation() const noexcept;
        void SetEyeLocation(const glm::vec3& new_eye_position);
        glm::vec3 GetViewDirection() const noexcept;
        void SetViewDirection(const glm::vec3& new_view_dir);
        void LookAtTarget(const glm::vec3& target_position);
        void LookTowardsTarget(const glm::vec3& viewer_position, const glm::vec3& target_position, const glm::vec3& new_up_vec = glm::vec3(0.0f));
        float GetFieldOfView() const noexcept;
        void SetFieldOfView(const float& new_fov);
        float GetHorizontalFieldOfView() const noexcept;
        void SetHorizontalFieldOfView(const float& new_horizontal_fov);
        glm::quat GetOrientation() const noexcept;
        void SetOrientation(const glm::quat& new_orientation);
        float GetFocalLength() const noexcept;
        float GetPivotDistance() const noexcept;
        void SetPivotDistance(const float& new_pivot_distance);
        glm::vec3 GetPivotPoint() const noexcept;
        float GetAspectRatio() const noexcept;
        void SetAspectRatio(const float& new_aspect);
        float GetNearClipPlaneDistance() const noexcept;
        void SetNearClipPlaneDistance(const float& new_clip_plane_dist);
        float GetFarClipPlaneDistance() const noexcept;
        void SetFarClipPlaneDistance(const float& new_clip_plane_distance);

        virtual void GetNearClipPlaneWorldspaceCoords(glm::vec3& top_left, glm::vec3& top_right, glm::vec3& bottom_left, glm::vec3& bottom_right) noexcept;
        virtual void GetFarClipPlaneWorldspaceCoords(glm::vec3& top_left, glm::vec3& top_right, glm::vec3& bottom_left, glm::vec3& bottom_right) noexcept;
        virtual void GetFrustumValues(float& left, float& top, float& right, float& bottom, float& _near, float& _far);
        Ray GetCameraRay(const float& u_pos, const float& v_pos, const float& image_plane_aspect_ratio) noexcept;
        Ray GetCameraRay(const glm::ivec2& point, const glm::ivec2& window_size) noexcept;
        void GetVectorsForBillboard(glm::vec3& right, glm::vec3& up) noexcept;

        glm::vec2 WorldspaceToScreen(const glm::vec3& world_position, const float& screen_width, const float& screen_height) noexcept;
        glm::vec2 EyespaceToScreen(const glm::vec3& eye_coord, const glm::ivec2& screen_size_in_pixels) noexcept;
        glm::vec3 WorldspaceToEyespace(const glm::vec3& world_position) noexcept;
        float WorldspaceToDepth(const glm::vec3& world_position) noexcept;
        glm::vec3 WorldspaceToNDC(const glm::vec3& world_position) noexcept;

        virtual const glm::mat4& GetProjectionMatrix() noexcept;
        virtual const glm::mat4& GetViewMatrix() noexcept;
        virtual const glm::mat4& GetInvViewMatrix() noexcept;

        virtual bool IsPerspective() const noexcept = 0;
        virtual void SetController(cameraController* new_controller);

    protected:

        cameraBase();

        virtual void updateMatrices();
        virtual void updateView();
        virtual void updateProjection();
        virtual void updateInvView();

        glm::vec3 eyePosition, eyeDirection;
        glm::quat orientation;
        glm::vec3 worldUp;
        glm::vec2 lensShift = glm::vec2(0.0f);
        float fieldOfView, aspectRatio;
        float nearClip, farClip;
        float frustumTop, frustumBottom, frustumLeft, frustumRight;
        float pivotDistance;
        glm::mat4 projection, invProjection, view, invView;
        glm::vec3 right, up, viewDirection, negViewDirection;
        cameraController* controller;
        bool viewCached, projectionCached, invViewCached, invProjectionCached;

	};

    class PerspectiveCamera : public cameraBase {
    public:

        PerspectiveCamera(const size_t& pixel_width, const size_t& pixel_height, const float& field_of_view);
        void SetPerspective(const float& vertical_fov_in_degrees, const float& aspect_ratio, const float& near_plane, const float& far_plane);
        virtual bool IsPerspective() const noexcept override;

    };

}

#endif // !VULPES_CAMERA_H