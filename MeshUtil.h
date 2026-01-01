#pragma once

#include "D3DUtil.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include <map>

class MeshUtil
{
public:
	static unique_ptr<MeshGeometry> CreateMesh(
		string name,
		map<string, GeometryGenerator::MeshData> meshs,
		ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* cmdList)
	{
		UINT vertexOffset = 0;
		UINT indexOffset = 0;
		UINT totalVertexCount = 0;

		map<string, SubmeshGeometry> submeshs;

		for (auto& meshPair : meshs)
		{
			auto& mesh = meshPair.second;
			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)mesh.Indices32.size();
			submesh.StartIndexLocation = indexOffset;
			submesh.BaseVertexLocation = vertexOffset;

			submeshs[meshPair.first] = submesh;

			indexOffset += (UINT)mesh.Indices32.size();
			vertexOffset += (UINT)mesh.Vertices.size();
		}

		vector<Vertex> vertices(vertexOffset);
		vector<uint16_t> indices;
		UINT index = 0;

		for (auto& meshPair : meshs)
		{
			auto& mesh = meshPair.second;
			for (size_t i = 0; i < mesh.Vertices.size(); ++i, ++index)
			{
				vertices[index].Pos = mesh.Vertices[i].Position;
				vertices[index].Normal = mesh.Vertices[i].Normal;
				vertices[index].TexC = mesh.Vertices[i].TexC;
			}
			indices.insert(indices.end(), begin(mesh.GetIndices16()), end(mesh.GetIndices16()));
		}

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

		auto geo = make_unique<MeshGeometry>();
		geo->Name = name;

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(d3dDevice,
			cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(d3dDevice,
			cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		for (auto& submesh : submeshs)
		{
			geo->DrawArgs[submesh.first] = submesh.second;
		}

		return geo;
	}

	static unique_ptr<MeshGeometry> LoadMesh(
		ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* cmdList,
		string name,
		wstring path)
	{
		ifstream fin(path);

		if (!fin)
		{
			wstring msg = path + L".txt not found.";
			MessageBox(0, msg.c_str(), 0, 0);
			return nullptr;
		}

		UINT vcount = 0;
		UINT tcount = 0;
		string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

		vector<Vertex> vertices(vcount);

		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

			XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

			XMFLOAT3 spherePos;
			XMStoreFloat3(&spherePos, XMVector3Normalize(P));

			float theta = atan2f(spherePos.z, spherePos.x);

			if(theta < 0.0f)
				theta += XM_2PI;

			float phi = acosf(spherePos.y);

			float u = theta / (2.0f * XM_PI);
			float v = phi / XM_PI;

			vertices[i].TexC = { u, v };

			vMin = XMVectorMin(vMin, P);
			vMax = XMVectorMax(vMax, P);
		}

		BoundingBox bounds;
		XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));

		fin >> ignore >> ignore >> ignore;

		vector<int32_t> indices(3 * tcount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(int32_t);

		auto geo = make_unique<MeshGeometry>();
		geo->Name = name;

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(d3dDevice,
			cmdList, vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(d3dDevice,
			cmdList, indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R32_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;
		submesh.Bounds = bounds;

		geo->DrawArgs[name] = submesh;

		return geo;
	}
};
