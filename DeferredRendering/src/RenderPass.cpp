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

#include "RenderPass.h"

using namespace ci;

RenderPass::~RenderPass(void)
{
}

RenderPassRef RenderPass::create(const ci::gl::Fbo::Format& format)
{
	return RenderPassRef( new RenderPass(format) );
}

void RenderPass::resize(int width, int height)
{
	// create the frame buffer
	mFrameBuffer = gl::Fbo( width >> mDownScaleSize, height >> mDownScaleSize, mFormat );
	
	if(mFormat.hasDepthBuffer())
		mFrameBuffer.getDepthTexture().setFlipped(true);
	
	for(int i=0;i<mFormat.getNumColorBuffers();++i)
		mFrameBuffer.getTexture(i).setFlipped(true);
}

void RenderPass::render()
{
	gl::SaveColorState			savedColorState;
	gl::SaveFramebufferBinding	savedFramebufferBinding;
	gl::SaveTextureBindState	savedTextureBindState( GL_TEXTURE_2D );

	glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

	mFrameBuffer.bindFramebuffer();

	gl::setViewport( mFrameBuffer.getBounds() );
	gl::pushMatrices();
	gl::setMatricesWindow( mFrameBuffer.getSize(), true );
	{
		if(mInputShader)
		{
			mInputShader.bind();
			// TODO: call callback to set uniforms (how?)
		}

		gl::drawSolidRect( mFrameBuffer.getBounds() );

		if(mInputShader)
			mInputShader.unbind();
	}
	gl::popMatrices();

	mFrameBuffer.unbindFramebuffer();

	glPopAttrib();
}

void RenderPass::render(const ci::CameraPersp& camera)
{
	gl::SaveColorState			savedColorState;
	gl::SaveFramebufferBinding	savedFramebufferBinding;
	gl::SaveTextureBindState	savedTextureBindState( GL_TEXTURE_2D );

	glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

	mFrameBuffer.bindFramebuffer();
	gl::setViewport( mFrameBuffer.getBounds() );
	gl::clear();

	if(mFrameBuffer.getFormat().hasDepthBuffer()) 
	{
		gl::enableDepthRead();
		gl::enableDepthWrite();
	}

	gl::pushMatrices();
	gl::setMatrices(camera);
	{
		if(mInputShader)
		{
			mInputShader.bind();
			// TODO: call callback to set uniforms (how?)
		}

		// 
		gl::color( Color::white() );

		// render meshes
		for( auto itr=mInputMeshes.begin(); itr != mInputMeshes.end(); ++itr )
			(*itr)->render();

		if(mInputShader)
			mInputShader.unbind();
	}
	gl::popMatrices();

	mFrameBuffer.unbindFramebuffer();

	glPopAttrib();
}

void RenderPass::attachTexture(int slot, ci::gl::TextureRef texture)
{
}

void RenderPass::detachTexture(int slot)
{
}

void RenderPass::addMesh(MeshRef mesh)
{
	mInputMeshes.push_back(mesh);
}

void RenderPass::removeMesh(MeshRef mesh)
{
	auto itr = std::find( mInputMeshes.begin(), mInputMeshes.end(), mesh );
	if(itr != mInputMeshes.end()) 
		mInputMeshes.erase( itr );
}

void RenderPass::loadShader(const ci::DataSourceRef vertex, const ci::DataSourceRef fragment)
{
	mInputShader = gl::GlslProg( vertex, fragment );
}

void RenderPass::loadShader(const ci::DataSourceRef vertex,  const ci::DataSourceRef geometry, const ci::DataSourceRef fragment)
{
}

ci::gl::Texture RenderPass::getTexture(int slot)
{
	if(mFrameBuffer && slot < mFormat.getNumColorBuffers())
		return mFrameBuffer.getTexture(slot);

	return gl::Texture();
}

ci::gl::Texture RenderPass::getDepthTexture()
{
	return gl::Texture();
}

