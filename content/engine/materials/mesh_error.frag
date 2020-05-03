#version 410 core

//! #include "includes/mesh_frag_header.glsl"

void main()
{
    float chessboard = floor(position_worldspace.x) + floor(position_worldspace.y) + floor(position_worldspace.z);
    chessboard = fract(chessboard * 0.5f);
    chessboard *= 2.0f;
    color = vec3(1.0f, 0.0f, 1.0f) * chessboard;
}