#pragma once

#include "cinder/TriMesh.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"

namespace ph { namespace render {

typedef std::shared_ptr<class Mesh> MeshRef;

class Mesh
{
public:
	Mesh(void);
	~Mesh(void);

	static MeshRef			create();
	static MeshRef			create( const ci::fs::path& mshFile );

	void					render( bool textured = true );

	ci::TriMesh&			getMesh() { return mTriMesh; }
	ci::gl::VboMeshRef		getVertexBuffer() { return mVboMeshRef; }

	const ci::Vec3f&		getPosition() const { return mPosition; }
	void					setPosition(float x, float y, float z);
	void					setPosition(const ci::Vec3f& position);	

	const ci::Quatf&		getOrientation() const { return mOrientation; }	
	void					setOrientation(float radiansX, float radiansY, float radiansZ);
	void					setOrientation(const ci::Vec3f& radians);
	void					setOrientation(const ci::Quatf& orientation);

	const ci::Vec3f&		getScale() const { return mScale; }
	float					getUnitScale() const;
	void					setScale(float scale);
	void					setScale(float x, float y, float z);
	void					setScale(const ci::Vec3f& scale);

	const ci::Matrix44f&	getTransform() const;

	bool					isDiffuseMapEnabled() const { return mDiffuseMapRef != ci::gl::TextureRef() && bUseDiffuseMap; }
	void					enableDiffuseMap( bool enable = true ) { bUseDiffuseMap = enable; }
	ci::gl::TextureRef		getDiffuseMap() { return mDiffuseMapRef; }
	void					setDiffuseMap(const ci::ImageSourceRef source) { mDiffuseMapRef = ci::gl::Texture::create(source); }

	bool					isSpecularMapEnabled() const { return mSpecularMapRef != ci::gl::TextureRef() && bUseSpecularMap; }
	void					enableSpecularMap( bool enable = true ) { bUseSpecularMap = enable; }
	ci::gl::TextureRef		getSpecularMap() { return mSpecularMapRef; }
	void					setSpecularMap(const ci::ImageSourceRef source) { mSpecularMapRef = ci::gl::Texture::create(source); }

	bool					isNormalMapEnabled() const { return mNormalMapRef != ci::gl::TextureRef() && bUseNormalMap; }
	void					enableNormalMap( bool enable = true ) { bUseNormalMap = enable; }
	ci::gl::TextureRef		getNormalMap() { return mNormalMapRef; }
	void					setNormalMap(const ci::ImageSourceRef source) { mNormalMapRef = ci::gl::Texture::create(source); }

	bool					isEmissiveMapEnabled() const { return mEmissiveMapRef != ci::gl::TextureRef() && bUseEmissiveMap; }
	void					enableEmissiveMap( bool enable = true ) { bUseEmissiveMap = enable; }
	ci::gl::TextureRef		getEmissiveMap() { return mEmissiveMapRef; }
	void					setEmissiveMap(const ci::ImageSourceRef source) { mEmissiveMapRef = ci::gl::Texture::create(source); }

	void					enableDebugging( bool enable = true );
	void					disableDebugging();

	void					readMesh( const ci::fs::path& mshFile, bool writeIfChanged = false );
	void					writeMesh( const ci::fs::path& mshFile );

private:
	bool					createMesh();
	bool					createDebugMesh();

private:
	ci::TriMesh				mTriMesh;
	ci::AxisAlignedBox3f	mBoundingBox;

	ci::Vec3f				mPosition;
	ci::Quatf				mOrientation;
	ci::Vec3f				mScale;
	mutable ci::Matrix44f	mTransform;
	mutable bool			mIsTransformDirty;

	ci::gl::VboMeshRef		mVboMeshRef;
	ci::gl::VboMeshRef		mVboDebugMeshRef;
	bool					mIsDebugEnabled;

	ci::gl::TextureRef		mDiffuseMapRef;
	ci::gl::TextureRef		mSpecularMapRef;
	ci::gl::TextureRef		mNormalMapRef;
	ci::gl::TextureRef		mEmissiveMapRef;

public:
	bool					bUseDiffuseMap;
	bool					bUseSpecularMap;
	bool					bUseNormalMap;
	bool					bUseEmissiveMap;
};

} } // namespace ph::render