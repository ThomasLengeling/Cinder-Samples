/*
 Copyright (c) 2013, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

#include "Mesh.h"

class RenderPass
{
public:
	enum DownScaleSize { ORIGINAL = 0, HALF, QUARTER, EIGHT };

	RenderPass(void) : mDownScaleSize(ORIGINAL), mNumColorBuffers(1), mHasDepthBuffer(true) {}
	~RenderPass(void);

	void	resize(int width, int height);
	void	render(const ci::CameraPersp& camera);
	void	renderFullScreen();

	void	attachTexture(int slot, ci::gl::TextureRef texture);
	void	detachTexture(int slot);

	void	addMesh(MeshRef mesh);
	void	removeMesh(MeshRef mesh);

	void	loadShader(const ci::fs::path& vertex, const ci::fs::path& fragment);
	void	loadShader(const ci::fs::path& vertex,  const ci::fs::path& geometry, const ci::fs::path& fragment);
	void	setShaderCallback() {} // TODO: will call supplied function with shader as param to set uniforms
	void	resetShaderCallback() {}

	ci::gl::TextureRef	getTexture(int slot) const;
	ci::gl::TextureRef	getDepthTexture() const;

private:
	std::vector<ci::gl::TextureRef>	mInputTextures;
	std::vector<MeshRef>			mInputMeshes;
	ci::gl::GlslProg				mInputShader;

	DownScaleSize					mDownScaleSize;

	int								mNumColorBuffers;
	bool							mHasDepthBuffer;

	ci::gl::Fbo						mFrameBuffer;
};

