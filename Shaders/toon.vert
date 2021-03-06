////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
// simple vertex shader

varying float fade;

void main()
	{	
		//Transform the vertex (ModelViewProj matrix)
		//gl_Position = ftransform();
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		fade = (gl_Vertex.z) / 40.0;
	}
