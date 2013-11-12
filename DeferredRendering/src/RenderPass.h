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

/////////////////////////////////////////////////////////////////////////////////////

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
	virtual ~RenderPass(void) {}

	void					clear( const ci::ColorA& color = ci::Color::black() );

	virtual void			resize(int width, int height) = 0;
	//! Renders a 3D pass.
	virtual void			render(const ci::CameraPersp& camera) = 0;
	//! Renders a full screen pass.
	virtual void			render() = 0;

	virtual void			loadShader() = 0;

	void					attachTexture(uint32_t slot, ci::gl::Texture& texture);
	void					detachTexture(uint32_t slot);

	void					addMesh(MeshRef mesh);
	void					removeMesh(MeshRef mesh);

	const ci::gl::Fbo::Format&	getFormat() const { return mFormat; }

	ci::gl::Texture&		getTexture(uint32_t slot);
	ci::gl::Texture&		getDepthTexture();

	void					setFlipped( bool flip = true );

	void					setDownScaleSize(DownScaleSize size) { mDownScaleSize = size; }

protected:
	ci::gl::Fbo&			getFrameBuffer() { return mFrameBuffer; }

	virtual void			loadShader(const ci::DataSourceRef vertex, const ci::DataSourceRef fragment);
	virtual void			loadShader(const ci::DataSourceRef vertex, const ci::DataSourceRef fragment, 
								const ci::DataSourceRef geometry, GLint input = 0, GLint output = 4, GLint vertices = 0);

	ci::gl::GlslProg		getShader() { return mInputShader; }

private:
	std::vector<ci::gl::Texture>	mInputTextures;
	std::vector<MeshRef>			mInputMeshes;
	ci::gl::GlslProg				mInputShader;

	DownScaleSize					mDownScaleSize;

	ci::gl::Fbo::Format				mFormat;
	ci::gl::Fbo						mFrameBuffer;
};

/////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<class RenderPassWireframe> RenderPassWireframeRef;

class RenderPassWireframe : public RenderPass
{
public:
	RenderPassWireframe(void) : 
		RenderPass() {}
	RenderPassWireframe( const ci::gl::Fbo::Format& format ) : 
		RenderPass(format) { assert(false); /* not supported */ }

	static RenderPassWireframeRef create();

	void resize(int width, int height);
	void render(const ci::CameraPersp& camera);
	void render() { assert(false); /* not supported */ }

	void loadShader();
};

/////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<class RenderPassNormalDepth> RenderPassNormalDepthRef;

class RenderPassNormalDepth : public RenderPass
{
public:
	RenderPassNormalDepth(void) : 
		RenderPass() { assert(false); /* not supported */ }
	RenderPassNormalDepth( const ci::gl::Fbo::Format& format ) : 
		RenderPass(format) {}

	static RenderPassNormalDepthRef create();

	void resize(int width, int height);
	void render(const ci::CameraPersp& camera);
	void render() { assert(false); /* not supported */ }

	void loadShader();
};

/////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<class RenderPassSSAO> RenderPassSSAORef;

class RenderPassSSAO : public RenderPass
{
public:
	RenderPassSSAO(void) : 
		RenderPass() { assert(false); /* not supported */ }
	RenderPassSSAO( const ci::gl::Fbo::Format& format ) : 
		RenderPass(format) {}	

	static RenderPassSSAORef create();

	void resize(int width, int height);
	void render(const ci::CameraPersp& camera);
	void render() { assert(false); /* not supported */ }

	void loadShader();

private:
	bool resizeSsaoKernel();
	bool resizeSsaoNoise();

public:
	static const int	kSsaoNoiseSize = 2;
	static const int	kSsaoKernelSize = 16;
	
	ci::gl::TextureRef	mTextureNoise;
};

/////////////////////////////////////////////////////////////////////////////////////

typedef std::shared_ptr<class RenderPassComposite> RenderPassCompositeRef;

class RenderPassComposite : public RenderPass
{
public:
	RenderPassComposite(void) : 
		RenderPass(),
		bUseDiffuseMap(true),
		bUseSpecularMap(true),
		bUseNormalMap(true),
		bUseEmmisiveMap(true),
		bShowNormalMap(false)
	{}
	RenderPassComposite( const ci::gl::Fbo::Format& format ) : 
		RenderPass(format) { assert(false); /* not supported */ }

	static RenderPassCompositeRef create();

	void resize(int width, int height);
	void render(const ci::CameraPersp& camera);
	void render() { assert(false); /* not supported */ }

	void loadShader();

public:
	bool bUseDiffuseMap;
	bool bUseSpecularMap;
	bool bUseNormalMap;
	bool bUseEmmisiveMap;
	bool bShowNormalMap;
};

