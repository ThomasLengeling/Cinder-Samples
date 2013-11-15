#include "Shader.h"
#include "cinder/app/AppBasic.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ph;
using namespace ph::render;

Shader::Shader(void) :
	bHasGeometryShader(false)
{
}

Shader::Shader(const std::string& name) :
	bHasGeometryShader(false)
{
	mVertexFile = name + "_vert.glsl";
	mFragmentFile = name + "_frag.glsl";
	mGeometryFile = name + "_geom.glsl";
}

Shader::~Shader(void)
{
}

ShaderRef Shader::create()
{
	return std::make_shared<Shader>();
}

ShaderRef Shader::create(const std::string& name)
{
	return std::make_shared<Shader>(name);
}

bool Shader::load()
{
	// get a reference to our path (and create it in the process)
	const fs::path& path = getPath();
	if(path.empty()) return false;

	// check if all files are present
	if(!fs::exists(path / mFragmentFile)) return false;
	
	bHasGeometryShader = fs::exists(path / mGeometryFile);
	//if(bHasGeometryShader) TODO: check if geometry settings are defined

	// parse source
	std::string vertexSource = parseShader( path / mVertexFile );
	//std::string fragmentSource = parseShader( path / mFragmentFile );


	//std::string geometrySource = parseShader( path / mGeometryFile );

	return true;
}

const fs::path& Shader::getPath() const
{
	// return path if already found
	if( !mPath.empty() ) return mPath;

	// find path:
	//  1. in assets folder
	//  2. in assets/shaders folder
	//  3. next to executable
	mPath = app::getAssetPath(mVertexFile);
	if( !mPath.empty() ) return mPath.parent_path();

	mPath = app::getAssetPath( fs::path("shaders") / mVertexFile );
	if( !mPath.empty() ) return mPath.parent_path();

	mPath = app::getAppPath() / mVertexFile;
	if( fs::exists(mPath) ) return mPath.parent_path();

	// not found (throw exception?)
	mPath.clear();
	return mPath;
}

std::string Shader::parseShader( const fs::path& path, bool optional, int level )
{
	std::stringstream output;

	if( level > 32 )
	{
		// TODO throw exception for cyclic inclusion
		return std::string();
	}

	static const boost::regex includeRegexp( "^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*" );

	std::ifstream input( path.c_str() );
	if( !input.is_open() )
	{
		if( optional )
		{
			return std::string();
		}

		if( level == 0 )
		{
			throw ci::Exception();	// Shader file not found or can't open it
		}
		else
		{
			throw ci::Exception();	// Shader include file not found or can't open it
		}

		return std::string();
	}

	// go through each line and process includes
	std::string		line;
	boost::smatch	matches;

	while( std::getline( input, line ) )
	{
		if( boost::regex_search( line, matches, includeRegexp ) )
		{
			output << parseShader( path / matches[1].str(), false, level + 1 );
		}
		else
		{
			output << line;
		}

		output << std::endl;
	}

	input.close();

	return output.str();
}
