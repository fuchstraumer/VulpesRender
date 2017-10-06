#include "util/Camera.hpp"
#include "util/MatrixUtils.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/rotate_normalized_axis.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace vulpes {


    constexpr float CAMERA_FLOAT_PI = 3.14159265359f;

    constexpr float lerp(const float& s, const float& e, const float& t) {
        return s + (e - s) * t;
    }

    glm::vec3 cameraBase::GetEyeLocation() const noexcept {
        return eyePosition;
    }

    void cameraBase::SetEyeLocation(const glm::vec3& new_eye_position) {
        viewCached = false;
        eyePosition = new_eye_position;
    }

    glm::vec3 cameraBase::GetViewDirection() const noexcept {
        return viewDirection;
    }

    void cameraBase::SetViewDirection(const glm::vec3 & new_view_dir) {
        viewDirection = glm::normalize(new_view_dir);
        negViewDirection = -viewDirection;
        orientation = glm::rotation(viewDirection, glm::vec3(0.0f, 0.0f, -1.0f));
        viewCached = false;
    }

    void cameraBase::LookAtTarget(const glm::vec3 & target_position) {
        viewDirection = glm::normalize(target_position - eyePosition);
        orientation = glm::toQuat(AlignZAxisWithTarget(viewDirection, worldUp));
        pivotDistance = glm::distance(target_position, eyePosition);
        viewCached = false;
    }

    void cameraBase::LookTowardsTarget(const glm::vec3& eye_position, const glm::vec3& target_position, const glm::vec3& new_up_vector) {
        eyePosition = eye_position;
        // This method just updates eye pos and potentially up dir before calling LookAtTarget.
        if (new_up_vector == glm::vec3(0.0f)) {
            LookAtTarget(target_position);
        }
        else {
            worldUp = glm::normalize(new_up_vector);
            LookAtTarget(target_position);
        }
    }

    float cameraBase::GetFieldOfView() const noexcept {
        return fieldOfView;
    }

    void cameraBase::SetFieldOfView(const float& new_field_of_view) {
        fieldOfView = new_field_of_view;
        projectionCached = false;
    }

    float cameraBase::GetHorizontalFieldOfView() const noexcept {
        return glm::degrees(2.0f * std::atan(std::tan(glm::radians(fieldOfView) * 0.5f) * aspectRatio));
    }

    void cameraBase::SetHorizontalFieldOfView(const float& new_horizontal_fov) {
        fieldOfView = glm::degrees(2.0f * std::atan(std::tan(glm::radians(new_horizontal_fov) * 0.5f) * aspectRatio));
    }

    glm::quat cameraBase::GetOrientation() const noexcept {
        return orientation;
    }

    void cameraBase::SetOrientation(const glm::quat& new_orientation) {
        orientation = glm::normalize(new_orientation);
        viewDirection = glm::rotate(orientation, glm::vec3(0.0f, 0.0f, -1.0f));
        viewCached = false;
    }

    float cameraBase::GetFocalLength() const noexcept {
        return 1.0f / (std::tanf(glm::radians(fieldOfView)*0.50f) * 2.0f);
    }

    float cameraBase::GetPivotDistance() const noexcept {
        return pivotDistance;
    }

    void cameraBase::SetPivotDistance(const float & new_pivot_distance) {
        pivotDistance = new_pivot_distance;
    }

    glm::vec3 cameraBase::GetPivotPoint() const noexcept {
        return glm::vec3(eyePosition + viewDirection * pivotDistance);
    }

    float cameraBase::GetAspectRatio() const noexcept {
        return aspectRatio;
    }

    void cameraBase::SetAspectRatio(const float & new_aspect) {
        aspectRatio = new_aspect;
        projectionCached = false;
    }

    float cameraBase::GetNearClipPlaneDistance() const noexcept {
        return nearClip;
    }

    void cameraBase::SetNearClipPlaneDistance(const float & new_clip_plane_dist) {
        nearClip = new_clip_plane_dist;
        projectionCached = false;
    }

    float cameraBase::GetFarClipPlaneDistance() const noexcept {
        return farClip;
    }

    void cameraBase::SetFarClipPlaneDistance(const float& far_clip_plane_dist) {
        farClip = far_clip_plane_dist;
        projectionCached = false;
    }

    void cameraBase::GetNearClipPlaneWorldspaceCoords(glm::vec3 & top_left, glm::vec3 & top_right, glm::vec3 & bottom_left, glm::vec3 & bottom_right) noexcept {
        updateMatrices();

        glm::vec3 view_dir = glm::normalize(viewDirection);

        top_left = eyePosition + (nearClip * view_dir) + (frustumTop * up) + (frustumLeft * right);
        top_right = eyePosition + (nearClip * view_dir) + (frustumTop * up) + (frustumRight * right);
        bottom_left = eyePosition + (nearClip * view_dir) + (frustumBottom * up) + (frustumLeft * right);
        bottom_right = eyePosition + (nearClip * view_dir) + (frustumBottom * up) + (frustumLeft * right);

    }

    void cameraBase::GetFarClipPlaneWorldspaceCoords(glm::vec3 & top_left, glm::vec3 & top_right, glm::vec3 & bottom_left, glm::vec3 & bottom_right) noexcept {
        updateMatrices();

        glm::vec3 view_dir = glm::normalize(viewDirection);
        float ratio = farClip / nearClip;

        top_left = eyePosition + (farClip * view_dir) + (ratio * frustumTop * up) + (frustumLeft * right);
        top_right = eyePosition + (farClip * view_dir) + (ratio * frustumTop * up) + (frustumRight * right);
        bottom_left = eyePosition + (farClip * view_dir) + (ratio * frustumBottom * up) + (frustumLeft * right);
        bottom_right = eyePosition + (farClip * view_dir) + (ratio * frustumBottom * up) + (frustumLeft * right);
    }

    void cameraBase::GetFrustumValues(float & left, float & top, float & right, float & bottom, float & _near, float & _far) {
        updateMatrices();

        left = frustumLeft;
        right = frustumRight;
        top = frustumTop;
        bottom = frustumBottom;
        _near = nearClip;
        _far = farClip;

    }

    Ray cameraBase::GetCameraRay(const float & u_pos, const float & v_pos, const float & image_plane_aspect_ratio) noexcept {
        updateMatrices();
        const float s = (u_pos - 0.50f) * image_plane_aspect_ratio;
        const float t = (v_pos - 0.50f);
        const float view_distance = image_plane_aspect_ratio / std::fabsf(frustumRight - frustumLeft) * nearClip;
        return Ray(eyePosition, glm::normalize(right * s + up * t - (negViewDirection * view_distance)));
    }

    Ray cameraBase::GetCameraRay(const glm::ivec2 & point, const glm::ivec2 & window_size) noexcept {
        return GetCameraRay(static_cast<float>(point.x / window_size.x), static_cast<float>((window_size.y - point.y) / window_size.y), static_cast<float>(window_size.x / window_size.y));
    }

    void cameraBase::GetVectorsForBillboard(glm::vec3 & right, glm::vec3 & up) noexcept {
        right = glm::vec3(glm::row(GetViewMatrix(), 0));
        up = glm::vec3(glm::row(GetViewMatrix(), 1));
    }

    glm::vec2 cameraBase::WorldspaceToScreen(const glm::vec3 & world_position, const float & screen_width, const float & screen_height) noexcept {
        const glm::vec4 eye_coords = GetViewMatrix() * glm::vec4(world_position, 1.0f);
        glm::vec4 ndc = GetProjectionMatrix() * eye_coords;
        
        ndc.x /= ndc.w;
        ndc.y /= ndc.w;
        ndc.z /= ndc.w;

        return glm::vec2((ndc.x + 1.0f) / 2.0f * screen_width, (1.0f - (ndc.y + 1.0f) / 2.0f) * screen_height);
    }

    glm::vec2 cameraBase::EyespaceToScreen(const glm::vec3 & eye_coord, const glm::ivec2 & screen_size_in_pixels) noexcept {
        glm::vec4 ndc = GetProjectionMatrix() * glm::vec4(eye_coord, 1.0f);
        ndc.x /= ndc.w;
        ndc.y /= ndc.w;
        ndc.z /= ndc.w;

        return glm::vec2(
            ((ndc.x + 1.0f) / 2.0f) * static_cast<float>(screen_size_in_pixels.x),
            (1.0f - (ndc.y + 1.0f) / 2.0f) * static_cast<float>(screen_size_in_pixels.y)
            );
    }

    glm::vec3 cameraBase::WorldspaceToEyespace(const glm::vec3 & world_position) noexcept {
        return glm::vec3(GetViewMatrix() * glm::vec4(world_position, 1.0f));
    }

    float cameraBase::WorldspaceToDepth(const glm::vec3 & world_position) noexcept {
        const glm::mat4& matrix = GetViewMatrix();
        return matrix[0][2] * world_position.x +
               matrix[1][2] * world_position.y +
               matrix[2][2] * world_position.z +
               matrix[3][2];
    }

    glm::vec3 cameraBase::WorldspaceToNDC(const glm::vec3 & world_position) noexcept {
        const glm::vec4 eye = GetViewMatrix() * glm::vec4(world_position, 1.0f);
        const glm::vec4 unprojected = GetProjectionMatrix() * eye;
        return glm::vec3(unprojected.x / unprojected.w, unprojected.y / unprojected.w, unprojected.z / unprojected.w);
    }

    const glm::mat4 & cameraBase::GetProjectionMatrix() noexcept {
        if (!projectionCached) {
            updateProjection();
        }
        return projection;
    }

    const glm::mat4 & cameraBase::GetViewMatrix() noexcept {
        if (!viewCached) {
            updateView();
        }
        return view;
    }

    const glm::mat4 & cameraBase::GetInvViewMatrix() noexcept {
        if (!invViewCached) {
            updateInvView();
        }
        return invView;
    }

    void cameraBase::SetController(cameraController * new_controller) {
        controller = new_controller;
    }

    cameraBase::cameraBase() : viewCached(false), projectionCached(false), invViewCached(false), worldUp(glm::vec3(0.0f, 1.0f, 0.0f)), pivotDistance(0.0f) {}

    void cameraBase::updateMatrices() {
        if (!viewCached) {
            updateView();
        }
        if (!projectionCached) {
            updateProjection();
        }
    }

    void cameraBase::updateView() {
        negViewDirection = -1.0f * glm::normalize(viewDirection);
        right = glm::rotate(orientation, glm::vec3(1.0f, 0.0f, 0.0f));
        up = glm::rotate(orientation, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::lookAt(eyePosition, eyePosition + viewDirection, up);
        //view = glm::lookAt(negViewDirection, eyePosition, up);

        viewCached = true;
        invViewCached = false;
    }

    void cameraBase::updateProjection() {
        frustumTop = nearClip * std::tan(CAMERA_FLOAT_PI / 180.0f * fieldOfView * 0.50f);
        frustumBottom = -frustumTop;
        frustumRight = frustumTop * aspectRatio;
        frustumLeft = -frustumRight;

        if (lensShift.y != 0.0f) {
            frustumTop = lerp(0.0f, 2.0f * frustumTop, 0.5f + 0.5f * lensShift.y);
            frustumBottom = lerp(2.0f * frustumBottom, 0.0f, 0.5f + 0.5f * lensShift.y);
        }

        if (lensShift.x != 0.0f) {
            frustumRight = lerp(2.0f * frustumRight, 0.0f, 0.5f - 0.5f * lensShift.x);
            frustumLeft = lerp(0.0f, 2.0f * frustumLeft, 0.5f - 0.5f * lensShift.x);
        }

        projection = glm::perspective(fieldOfView, aspectRatio, nearClip, farClip);
        projection[1][1] *= -1.0f;
        invProjection = glm::inverse(projection);

        projectionCached = true;

    }

    void cameraBase::updateInvView() {
        if (!viewCached) {
            updateView();
        }
        invView = glm::inverse(view);
        invViewCached = true;
    }

    PerspectiveCamera::PerspectiveCamera(const size_t & pixel_width, const size_t & pixel_height, const float & field_of_view) : cameraBase() {
        glm::vec2 eye_xy(static_cast<float>(pixel_height / 2), static_cast<float>(pixel_width / 2));
        float half_fov = CAMERA_FLOAT_PI * (field_of_view / 360.0f);
        float tan_fov = std::tan(half_fov);
        float dist = eye_xy.y / tan_fov;
        float near_dist = 0.1f;
        float far_dist = 4000.0f;
        float aspect = static_cast<float>(pixel_width) / static_cast<float>(pixel_height);
        SetPerspective(field_of_view, aspect, near_dist, far_dist);
        LookTowardsTarget(glm::vec3(0.0f, 10.0f, 20.0f), glm::vec3(0.0f));
    }

    void PerspectiveCamera::SetPerspective(const float & vertical_fov_in_degrees, const float & aspect_ratio, const float & near_plane, const float & far_plane) {
        fieldOfView = vertical_fov_in_degrees;
        aspectRatio = aspect_ratio;
        nearClip = near_plane;
        farClip = far_plane;
        projectionCached = false;
    }

    void PerspectiveCamera::MouseDrag(const int & button, const float & x_offset, const float & y_offset)
    {
    }

    void PerspectiveCamera::MouseScroll(const int & button, const float & y_scroll)
    {
    }

    void PerspectiveCamera::MouseDown(const int & button, const float & x, const float & y)
    {
    }

    void PerspectiveCamera::MouseUp(const int & button, const float & x, const float & y)
    {
    }

    bool PerspectiveCamera::IsPerspective() const noexcept {
        return true;
    }

}