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

typedef std::shared_ptr<class RenderPass> RenderPassRef;

class RenderPass
{
public:
	enum DownScaleSize { ORIGINAL = 0, HALF, QUARTER, EIGHT };

	RenderPass(void) : 
		mDownScaleSize(ORIGINAL) {}
	RenderPass( const ci::gl::Fbo::Format& format ) : 
		mDownScaleSize(ORIGINAL),
		mFormat(format) {}
	~RenderPass(void);

	static RenderPassRef	create( const ci::gl::Fbo::Format& format );

	void	resize(int width, int height);
	void	render(const ci::CameraPersp& camera);
	void	render();

	void	attachTexture(int slot, ci::gl::TextureRef texture);
	void	detachTexture(int slot);

	void	addMesh(MeshRef mesh);
	void	removeMesh(MeshRef mesh);

	void	loadShader(const ci::DataSourceRef vertex, const ci::DataSourceRef fragment);
	void	loadShader(const ci::DataSourceRef vertex,  const ci::DataSourceRef geometry, const ci::DataSourceRef fragment);
	void	setShaderCallback() {} // TODO: will call supplied function with shader as param to set uniforms
	void	resetShaderCallback() {}

	ci::gl::GlslProg	getShader() { return mInputShader; }

	ci::gl::Texture		getTexture(int slot);
	ci::gl::Texture		getDepthTexture();

	void	setDownScaleSize(DownScaleSize size) { mDownScaleSize = size; }

private:
	std::vector<ci::gl::TextureRef>	mInputTextures;
	std::vector<MeshRef>			mInputMeshes;
	ci::gl::GlslProg				mInputShader;

	DownScaleSize					mDownScaleSize;

	ci::gl::Fbo::Format				mFormat;
	ci::gl::Fbo						mFrameBuffer;
};

