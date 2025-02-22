//
// Created by kj16609 on 6/14/23.
//

#include <filesystem>
#include <fstream>
#include <lz4frame.h>

#include <tracy/Tracy.hpp>

#include "core/logging.hpp"

namespace atlas
{

	std::size_t get_block_size( const LZ4F_frameInfo_t* info )
	{
		switch ( info->blockSizeID )
		{
			default:
				throw std::runtime_error( "Invalid block size" );
			case LZ4F_default:
				[[fallthrough]];
			case LZ4F_max64KB:
				return 1 << 16;
			case LZ4F_max256KB:
				return 1 << 18;
			case LZ4F_max1MB:
				return 1 << 20;
			case LZ4F_max4MB:
				return 1 << 22;
		}
	}

	std::vector< char > extract( const std::filesystem::path path )
	{
		ZoneScoped;
		spdlog::info( "Extracting {}", path );
		LZ4F_dctx* dctx { nullptr };
		if ( const auto status = LZ4F_createDecompressionContext( &dctx, LZ4F_VERSION ); LZ4F_isError( status ) )
		{
			throw std::runtime_error(
				fmt::format( "Failed to create decompression context: {}", LZ4F_getErrorName( status ) ) );
		}

		std::vector< char > out_data;
		out_data.reserve( std::filesystem::file_size( path ) );

		if ( std::ifstream ifs( path, std::ios::binary ); ifs )
		{
			//Read header
			std::array< char, LZ4F_HEADER_SIZE_MAX > header_buffer;
			ifs.read( header_buffer.data(), header_buffer.size() );

			std::size_t processed_bytes { LZ4F_HEADER_SIZE_MAX };

			LZ4F_frameInfo_t info;
			if ( const auto fires = LZ4F_getFrameInfo( dctx, &info, header_buffer.data(), &processed_bytes );
			     LZ4F_isError( fires ) )
			{
				throw std::runtime_error( fmt::format( "Failed to get frame info: {}", LZ4F_getErrorName( fires ) ) );
			}

			const auto block_size { get_block_size( &info ) };

			char* const decompression_buffer = new char[ block_size ];
			std::memset( decompression_buffer, 0, block_size );

			spdlog::info( "Header ate {} bytes", processed_bytes );
			//Rewind file
			ifs.seekg( static_cast< long long >( processed_bytes ), std::ios::beg );

			std::size_t ret { 1 };

			std::array< char, 1 << 16 > buffer;
			std::size_t bytes_left { 0 };

			while ( ret != 0 && ifs.good() && !ifs.eof() )
			{
				bytes_left += static_cast< size_t >(
					ifs.readsome( buffer.data() + bytes_left, static_cast< long >( buffer.size() - bytes_left ) ) );

				assert( bytes_left <= buffer.size() );

				//This counter gets overwritten with the number of bytes read from buffer
				// This counter is set with the number of bytes we have
				std::size_t in_buffer_bytes { bytes_left };

				//This counter gets overwritten with the number of bytes written to decomp buffer.
				// This counter is set with the output buffer size
				std::size_t out_buffer_bytes { block_size };

				if ( bytes_left == 0 ) break;
				ret = LZ4F_decompress(
					dctx, decompression_buffer, &out_buffer_bytes, buffer.data(), &in_buffer_bytes, nullptr );
				const auto bytes_processed { in_buffer_bytes };

				if ( LZ4F_isError( ret ) )
				{
					spdlog::error( "Failed to decompress: {}", LZ4F_getErrorName( ret ) );
					throw std::runtime_error( fmt::format( "Failed to decompress: {}", LZ4F_getErrorName( ret ) ) );
				}
				//Shift buffer over by ready_bytes (bytes read by LZ4F_decompress)
				std::memmove( buffer.data(), buffer.data() + bytes_processed, buffer.size() - bytes_processed );
				//Narrow down bytes_left to new range
				bytes_left = buffer.size() - bytes_processed;

				//Copy into out buffer
				const auto pre_size { out_data.size() };
				out_data.resize( out_data.size() + out_buffer_bytes );
				std::memcpy( out_data.data() + pre_size, decompression_buffer, out_buffer_bytes );
			}
		}
		else
			throw std::runtime_error( fmt::format( "Failed to open file: {}", path.string() ) );

		LZ4F_freeDecompressionContext( dctx );
		const auto file_size { std::filesystem::file_size( path ) };
		const auto data_size { out_data.size() };
		const float percent { static_cast< float >( data_size ) / static_cast< float >( file_size ) };
		spdlog::info(
			"Finished extracting file {} -> {}: {}%", std::filesystem::file_size( path ), out_data.size(), percent );
		return out_data;
	}

} // namespace atlas
