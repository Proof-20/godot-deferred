#include "scene_shader_deferred.h"

#include "core/error/error_macros.h"
#include "core/os/memory.h"

namespace RendererSceneRenderImplementation {
void SceneShaderDeferred::ShaderData::set_code(const String &p_code) {
	uniforms.clear();

	if (!p_code.is_empty())
		ERR_PRINT_ONCE("Deferred+: spatial shader compilation is not supported by the initialization backend. No scene shader was compiled.");
}

bool SceneShaderDeferred::ShaderData::is_animated() const {
	return false;
}

bool SceneShaderDeferred::ShaderData::casts_shadows() const {
	return false;
}

RenderingServerTypes::ShaderNativeSourceCode SceneShaderDeferred::ShaderData::get_native_source_code() const {
	return {};
}

Pair<ShaderRD *, RID> SceneShaderDeferred::ShaderData::get_native_shader_and_version() const {
	return {};
}

void SceneShaderDeferred::MaterialData::set_render_priority(const int p_priority) {
	render_priority = p_priority;
}

void SceneShaderDeferred::MaterialData::set_next_pass(RID p_pass) {
	next_pass = p_pass;
}

bool SceneShaderDeferred::MaterialData::update_parameters(const HashMap<StringName, Variant> &, bool, bool) {
	// This return value means "GPU uniform set changed", not shader success.
	// Rejected spatial shaders have no GPU uniform set to update.
	return false;
}

RendererRD::MaterialStorage::ShaderData * SceneShaderDeferred::_create_shader_data() {
	return memnew(ShaderData);
}

RendererRD::MaterialStorage::MaterialData * SceneShaderDeferred::_create_material_data(RendererRD::MaterialStorage::ShaderData *) {
	return memnew(MaterialData);
}

SceneShaderDeferred::SceneShaderDeferred() {
	RendererRD::MaterialStorage *storage = RendererRD::MaterialStorage::get_singleton();

	DEV_ASSERT(storage->material_get_data_request_function(RendererRD::MaterialStorage::SHADER_TYPE_3D) == nullptr);

	storage->shader_set_data_request_function(RendererRD::MaterialStorage::SHADER_TYPE_3D, _create_shader_data);
	storage->material_set_data_request_function(RendererRD::MaterialStorage::SHADER_TYPE_3D, _create_material_data);
}

SceneShaderDeferred::~SceneShaderDeferred() {
	RendererRD::MaterialStorage *storage = RendererRD::MaterialStorage::get_singleton();

	DEV_ASSERT(storage->material_get_data_request_function(RendererRD::MaterialStorage::SHADER_TYPE_3D) == _create_material_data);

	storage->shader_set_data_request_function(RendererRD::MaterialStorage::SHADER_TYPE_3D, nullptr);
	storage->material_set_data_request_function(RendererRD::MaterialStorage::SHADER_TYPE_3D, nullptr);
}

}
