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

void RenderPass::resize(int width, int height)
{
	// create the frame buffer
	gl::Fbo::Format fmt;
	fmt.enableColorBuffer(true, mNumColorBuffers);
	fmt.enableDepthBuffer(mHasDepthBuffer, true);
	fmt.setColorInternalFormat( GL_RGBA16F );
	fmt.setDepthInternalFormat( GL_DEPTH24_STENCIL8 );

	mFrameBuffer = gl::Fbo( width >> mDownScaleSize, height >> mDownScaleSize, fmt );
	mFrameBuffer.getDepthTexture().setFlipped(true);
	for(int i=0;i<mNumColorBuffers;++i)
		mFrameBuffer.getTexture(i).setFlipped(true);
}

void RenderPass::render(const ci::CameraPersp& camera)
{
}

void RenderPass::renderFullScreen()
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

void RenderPass::attachTexture(int slot, ci::gl::TextureRef texture)
{
}

void RenderPass::detachTexture(int slot)
{
}

void RenderPass::addMesh(MeshRef mesh)
{
}

void RenderPass::removeMesh(MeshRef mesh)
{
}

void RenderPass::loadShader(const ci::fs::path& vertex, const ci::fs::path& fragment)
{
}

void RenderPass::loadShader(const ci::fs::path& vertex,  const ci::fs::path& geometry, const ci::fs::path& fragment)
{
}

ci::gl::TextureRef RenderPass::getTexture(int slot) const
{
	return gl::TextureRef();
}

ci::gl::TextureRef RenderPass::getDepthTexture() const
{
	return gl::TextureRef();
}

