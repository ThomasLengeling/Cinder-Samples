#pragma once

#include "cinder/Filesystem.h"

namespace ph { namespace render {

typedef std::shared_ptr<class Shader> ShaderRef;

class Shader
{
public:
	Shader(void);
	Shader(const std::string& name);
	~Shader(void);

	static ShaderRef		create();
	static ShaderRef		create( const std::string& name );

	bool					load();

	const ci::fs::path&		getPath() const;

protected:
	std::string parseShader( const ci::fs::path& path, bool optional = true, int level = 0 );

private:
	std::string		mVertexFile;
	std::string		mFragmentFile;
	std::string		mGeometryFile;

	bool			bHasGeometryShader;
	
	mutable ci::fs::path	mPath;
};

} } // namespace ph::render