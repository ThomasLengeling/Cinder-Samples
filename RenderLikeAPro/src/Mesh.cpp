#include "Mesh.h"

using namespace ci;
using namespace ph;
using namespace ph::render;

Mesh::Mesh(void) :
	mPosition( Vec3f::zero() ), 
	mOrientation( Quatf::identity() ),
	mScale( Vec3f::one() ),
	mIsTransformDirty( true ),
	mIsDebugEnabled( false ),
	bUseDiffuseMap(true),
	bUseSpecularMap(true),
	bUseNormalMap(true),
	bUseEmissiveMap(true)
{
}

Mesh::~Mesh(void)
{
}

MeshRef Mesh::create()
{
	return std::make_shared<Mesh>();
}

MeshRef Mesh::create( const ci::fs::path& mshFile )
{
	MeshRef mesh = std::make_shared<Mesh>();
	mesh->readMesh(mshFile);
	return mesh;
}

void Mesh::render( bool textured )
{
	//
	if(!mVboMeshRef && !createMesh())
		return;
	
	//
	gl::SaveTextureBindState savedTextureBindState( GL_TEXTURE_2D );

	if(textured)
	{

		if(mDiffuseMapRef)
			mDiffuseMapRef->bind(0);

		if(mSpecularMapRef)
			mSpecularMapRef->bind(1);

		if(mNormalMapRef)
			mNormalMapRef->bind(2);

		if(mEmissiveMapRef)
			mEmissiveMapRef->bind(3);

		glEnable( GL_TEXTURE_2D );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}

	gl::pushModelView();
	{
		gl::multModelView( getTransform() );
		gl::draw( mVboMeshRef );

		//
		if(mIsDebugEnabled && mVboDebugMeshRef)
		{
			GLint currentProgram;	
			glGetIntegerv( GL_CURRENT_PROGRAM, &currentProgram );
			glDisable( GL_TEXTURE_2D );

			glUseProgram( 0 );
			gl::draw( mVboDebugMeshRef );
			glUseProgram( currentProgram );
		}
	}
	gl::popModelView();
}

void Mesh::setPosition(float x, float y, float z)
{
	mPosition = Vec3f(x, y, z);
	mIsTransformDirty = true;
}

void Mesh::setPosition(const Vec3f& position)
{
	mPosition = position;
	mIsTransformDirty = true;
}

void Mesh::setOrientation(float radiansX, float radiansY, float radiansZ)
{
	mOrientation.set(radiansX, radiansY, radiansZ);
	mIsTransformDirty = true;
}

void Mesh::setOrientation(const Vec3f& radians)
{
	mOrientation.set(radians.x, radians.y, radians.z);
	mIsTransformDirty = true;
}

void Mesh::setOrientation(const Quatf& orientation)
{
	mOrientation = orientation;
	mIsTransformDirty = true;
}

float Mesh::getUnitScale() const
{
	Vec3f size = mBoundingBox.getSize();
	float scale = math<float>::max( size.x, math<float>::max( size.y, size.z ) );
	return (scale > 0.0f) ? 1.0f / scale : 1.0f;
}

void Mesh::setScale(float scale)
{
	mScale = Vec3f(scale, scale, scale);
	mIsTransformDirty = true;
}

void Mesh::setScale(float x, float y, float z)
{
	mScale = Vec3f(x, y, z);
	mIsTransformDirty = true;
}

void Mesh::setScale(const Vec3f& scale)
{
	mScale = scale;
	mIsTransformDirty = true;
}

const ci::Matrix44f& Mesh::getTransform() const
{
	if(mIsTransformDirty) 
	{
		mTransform.setToIdentity();
		mTransform.translate( mPosition );
		mTransform *= mOrientation.toMatrix44();
		mTransform.scale( mScale );

		mIsTransformDirty = false;
	}

	return mTransform;
}

void Mesh::enableDebugging( bool enable )
{
	mIsDebugEnabled = enable;

	if(mIsDebugEnabled && !mVboDebugMeshRef)
		createDebugMesh();
}

void Mesh::disableDebugging()
{
	mIsDebugEnabled = false;
}

void Mesh::readMesh( const ci::fs::path& mshFile, bool writeIfChanged )
{
	bool hasChanged = false;

	if(fs::exists(mshFile))
		mTriMesh.read( loadFile(mshFile) );

	// if the mesh does not have normals, calculate them on-the-fly
	if(!mTriMesh.hasNormals()) {
		mTriMesh.recalculateNormals();
		hasChanged = true;
	}

	// if the mesh does not have tangents, calculate them on-the-fly
	//  (note: your model needs to have normals and texture coordinates for this to work)
	if(!mTriMesh.hasTangents()) {
		mTriMesh.recalculateTangents(); 
		hasChanged = true;
	}

	// write file if changed
	if(writeIfChanged && hasChanged)
		mTriMesh.write( writeFile(mshFile) );

	// 
	mBoundingBox = mTriMesh.calcBoundingBox();

	//
	mVboMeshRef = gl::VboMeshRef();
	mVboDebugMeshRef = gl::VboMeshRef();
}

void Mesh::writeMesh( const ci::fs::path& mshFile )
{
	mTriMesh.write( writeFile(mshFile) );
}

bool Mesh::createMesh()
{
	if(mTriMesh.getNumVertices() == 0)
		return false;

	mVboMeshRef = gl::VboMesh::create( mTriMesh );
	return true;
}

bool Mesh::createDebugMesh()
{
	// create a debug mesh, showing normals, tangents and bitangents
	size_t numVertices = mTriMesh.getNumVertices();

	// determine the right scale, based on the bounding box
	AxisAlignedBox3f bbox = mTriMesh.calcBoundingBox();
	Vec3f size = bbox.getMax() - bbox.getMin();
	float scale = math<float>::max( math<float>::max( float(size.x), float(size.y) ), float(size.z) ) / 100.0f;

	if(numVertices > 0)
	{
		std::vector<Vec3f>		vertices;	vertices.reserve( numVertices * 4 );
		std::vector<Color>		colors;		colors.reserve( numVertices * 4 );
		std::vector<uint32_t>	indices;	indices.reserve( numVertices * 6 );

		for(size_t i=0;i<numVertices;++i) {
			uint32_t idx = vertices.size();

			vertices.push_back( mTriMesh.getVertices()[i] );
			vertices.push_back( mTriMesh.getVertices()[i] + scale * mTriMesh.getTangents()[i] );
			vertices.push_back( mTriMesh.getVertices()[i] + scale * mTriMesh.getNormals()[i].cross(mTriMesh.getTangents()[i]) );
			vertices.push_back( mTriMesh.getVertices()[i] + scale * mTriMesh.getNormals()[i] );

			colors.push_back( Color(0, 0, 0) );
			colors.push_back( Color(1, 0, 0) );
			colors.push_back( Color(0, 1, 0) );
			colors.push_back( Color(0, 0, 1) );

			indices.push_back( idx );
			indices.push_back( idx + 1 );
			indices.push_back( idx );
			indices.push_back( idx + 2 );
			indices.push_back( idx );
			indices.push_back( idx + 3 );
		}

		gl::VboMesh::Layout layout;
		layout.setStaticPositions();
		layout.setStaticColorsRGB();
		layout.setStaticIndices();
	
		mVboDebugMeshRef = gl::VboMesh::create( numVertices * 4, numVertices * 6, layout, GL_LINES );
		mVboDebugMeshRef->bufferPositions( vertices );
		mVboDebugMeshRef->bufferColorsRGB( colors );
		mVboDebugMeshRef->bufferIndices( indices );

		return true;
	}
	else
	{
		mVboDebugMeshRef = gl::VboMeshRef();

		return false;
	}
}