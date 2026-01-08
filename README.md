# GPUFrustumCulling

DirectX 12 기반의 ExecuteIndirect를 활용한 GPU Driven rendering 파이프라인


## 🌐 다국어 README (클릭하여 펼치기)

<details>
<summary><strong>🇰🇷 한국어 (Korean)</strong></summary>
<br>

### GPU Driven Frustum Culling System

#### 핵심 특징
- **GPU 기반 Frustum Culling** : Compute Shader를 사용하여 수만 개의 인스턴스 가시성을 GPU에서 병렬로 판단
- **ExecuteIndirect** 활용: CPU의 Draw Call 오버헤드를 완전히 제거하고, 한 번의 명령으로 모든 메쉬 렌더링
- **Instancing**: 동일한 메쉬의 여러 인스턴스를 효율적으로 처리
- **동적 오브젝트 관리**: 최대 2048개(16 commands × 128 objects)의 오브젝트 지원

#### 기술스택
| 분야 | 기술 |
|------|------|
| **Graphics API** | DirectX 12 |
| **Shaders** | HLSL (Compute, Vertex, Pixel) |
| **Rendering** | GPU Driven Pipeline, Indirect Drawing |
| **Language** | C++ |
| **Tools** | Visual Studio 2022 |

#### 프로젝트 구조
```
GPUFrustumCulling/
├── BaseApp.cpp/h              # 애플리케이션 기본 프레임워크
├── GPUFrustumCullingApp.cpp   # 메인 애플리케이션 클래스
├── GPUFrustumCulling.cpp/h    # GPU 컬링 로직 구현
├── FrustumCulling.cpp/h       # CPU 프러스텀 컬링 (비교용)
├── Camera.cpp/h               # 카메라 시스템
├── D3DApp.cpp/h               # DirectX 12 초기화 및 관리
├── FrameResource.cpp/h        # 프레임 리소스 관리
├── Shaders/
│   ├── GPUFrustumCulling.hlsl # Compute Shader 컬링 구현
│   ├── Default.hlsl           # 기본 렌더링 셰이더
│   └── Common.hlsl            # 공통 셰이더 함수
├── Models/
│   └── skull.txt              # 테스트용 3D 모델
└── Materials/
    └── defaultMat.txt         # 기본 머티리얼
```

**최적화 포인트**
* Command Signature 설계: 메쉬별로 다른 Vertex/Index Buffer를 명령 구조체에 포함하여 GPU가 스스로 버퍼를 교체하도록 구현
* 비동기 컴퓨트: Graphics와 Compute Queue 간의 Fence 동기화를 통해 안정적인 데이터 흐름 제어

#### 요구사항
* Windows 10/11
* Visual Studio 2022
* DirectX 12 지원 GPU
* Windows SDK 10.0.19041.0 이상

</details>

<details>
<summary><strong>🇯🇵 日本語 (Japanese)</strong></summary>
<br>
  
### GPU Driven Frustum Culling System
#### 主な特徴
  - GPUベースの視錐台カリング (Frustum Culling): Compute Shaderを使用して、数万個のインスタンスの可視性をGPU側で並列判定
  - ExecuteIndirectの活用: CPUのDraw Callオーバーヘッドを完全に排除し、単一のコマンドですべてのメッシュを描画
  - インスタンシング (Instancing): 同一メッシュの複数インスタンスを効率的に処理
  - 動的オブジェクト管理: 最大2048個 (16 commands × 128 objects) のオブジェクトをサポート
 
#### 技術スタック
| 分野 | 技術 |
|------|------|
| **Graphics API** | DirectX 12 |
| **Shaders** | HLSL (Compute, Geometry, Vertex, Pixel) |
| **Rendering** | GPU Driven Pipeline, Indirect Drawing |
| **Language** | C++ |
| **Tools** | Visual Studio 2022 |

#### プロジェクト構造
```
GPUFrustumCulling/
├── BaseApp.cpp/h               # アプリケーション基本フレームワーク
├── GPUFrustumCullingApp.cpp    # メインアプリケーションクラス
├── GPUFrustumCulling.cpp/h     # GPUカリングロジックの実装
├── FrustumCulling.cpp/h        # CPU視錐台カリング (比較用)
├── Camera.cpp/h                # カメラシステム
├── D3DApp.cpp/h                # DirectX 12初期化および管理
├── FrameResource.cpp/h         # フレームリソース管理
├── Shaders/
│   ├── GPUFrustumCulling.hlsl  # Compute Shaderカリングの実装
│   ├── Default.hlsl            # 基本レンダリングシェーダ
│   └── Common.hlsl             # 共通シェーダ関数
├── Models/
│   └── skull.txt               # テスト用3Dモデル
└── Materials/
    └── defaultMat.txt          # 基本マテリアル
```

#### 最適化ポイント
- Command Signatureの設計: メッシュごとに異なるVertex/Index Bufferをコマンド構造体に含め、GPUが自律的にバッファを切り替えるよう実装
- 非同期コンピューティング: GraphicsとCompute Queue間のFence同期により、安定したデータフローを制御

#### 要件
- Windows 10/11
- Visual Studio 2022
- DirectX 12対応GPU
- Windows SDK 10.0.19041.0以上
</details>

<details>
<summary><strong>🇬🇧 English</strong></summary>
<br>
  
### GPU Driven Frustum Culling System
#### Key Features
  - GPU-Based Frustum Culling: Determines the visibility of tens of thousands of instances in parallel using Compute Shaders.
  - ExecuteIndirect Integration: Completely eliminates CPU Draw Call overhead by rendering all meshes with a single command.
  - Instancing: Efficiently processes multiple instances of the same mesh.
  - Dynamic Object Management: Supports up to 2,048 objects (16 commands × 128 objects).

#### Tech Stack
| Category | Technology |
|------|------|
| **Graphics API** | DirectX 12 |
| **Shaders** | HLSL (Compute, Geometry, Vertex, Pixel) |
| **Rendering** | GPU Driven Pipeline, Indirect Drawing |
| **Language** | C++ |
| **Tools** | Visual Studio 2022 |

#### Project Structure
```
GPUFrustumCulling/
├── BaseApp.cpp/h               # Application Base Framework
├── GPUFrustumCullingApp.cpp    # Main Application Class
├── GPUFrustumCulling.cpp/h     # GPU Culling Logic Implementation
├── FrustumCulling.cpp/h        # CPU Frustum Culling (for comparison)
├── Camera.cpp/h                # Camera System
├── D3DApp.cpp/h                # DirectX 12 Initialization & Management
├── FrameResource.cpp/h         # Frame Resource Management
├── Shaders/
│   ├── GPUFrustumCulling.hlsl  # Compute Shader Culling Implementation
│   ├── Default.hlsl            # Basic Rendering Shader
│   └── Common.hlsl             # Common Shader Functions
├── Models/
│   └── skull.txt               # Test 3D Model
└── Materials/
    └── defaultMat.txt          # Basic Material
```
#### Optimization Points
- Command Signature Design: Included distinct Vertex/Index Buffers for each mesh within the command structure, allowing the GPU to swap buffers autonomously.
- Async Compute: Managed stable data flow through Fence synchronization between Graphics and Compute Queues.

#### Requirements
- Windows 10/11
- Visual Studio 2022
- DirectX 12 Compatible GPU
- Windows SDK 10.0.19041.0 or higher
</details>
