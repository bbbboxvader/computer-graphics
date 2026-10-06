# Computer Graphics 실습 9~12

OpenGL, GLFW, GLEW를 사용한 컴퓨터 그래픽스 실습입니다.

실습 9, 10, 11, 12는 각각 독립된 Visual Studio 실행 프로젝트로 구성되어 있습니다.

## 노트북에서 실행하기

### 1. 필요한 프로그램

- Windows 10 또는 Windows 11
- Visual Studio 2022
- Visual Studio 설치 항목에서 `C++를 사용한 데스크톱 개발`
- Windows 10/11 SDK

GLEW와 GLFW의 x64 헤더·라이브러리·실행 DLL은 `third_party` 폴더에 포함되어 있습니다. Windows SDK 폴더에 라이브러리를 따로 복사할 필요가 없습니다.

### 2. 저장소 받기

```powershell
git clone https://github.com/bbbboxvader/computer-graphics.git
cd computer-graphics
```

### 3. Visual Studio에서 실행하기

1. `컴그/컴그.slnx`를 Visual Studio 2022로 엽니다.
2. 상단 구성을 `Debug`, 플랫폼을 `x64`로 선택합니다.
3. 원하는 프로젝트를 마우스 오른쪽 버튼으로 누릅니다.
4. `시작 프로젝트로 설정`을 선택합니다.
5. `Ctrl + F5`를 누릅니다.

프로젝트와 실습 번호는 다음과 같습니다.

| 프로젝트 | 소스 파일 | 내용 |
|---|---|---|
| `Practice09` | `1-9.cpp` | 삼각형 2개의 네 가지 애니메이션 |
| `Practice10` | `1-10.cpp` | 도형을 모양판에 드래그해 맞추기 |
| `Practice11` | `1-11.cpp` | 20×20 보드 이동과 장애물 충돌 |
| `Practice12` | `1-12.cpp` | 두 사각형의 타이밍 맞추기 |

## 키 조작

### 실습 9

- `1`: 화면 벽에서 튕기기
- `2`: 가로 지그재그
- `3`: 상하 대각선 지그재그
- `4`: 미리 그려진 중심 스파이럴 경로 따라가기
- `A`: 채우기
- `B`: 테두리
- `R` 또는 `C`: 초기화
- `Q` 또는 `Esc`: 종료

### 실습 10

- 마우스 왼쪽 드래그: 도형 이동
- 정확한 자리에 놓인 도형은 잠기며 다시 움직이지 않음
- `R`: 새로운 문제로 초기화
- `Q` 또는 `Esc`: 종료

### 실습 11

- `Space` 또는 오른쪽 방향키: 한 칸 이동
- `A`: 자동 이동
- `R`: 초기화
- `Q` 또는 `Esc`: 종료

### 실습 12

- `Enter`: 두 사각형이 표시 구간에 있을 때 모양판으로 이동
- `R`: 초기화
- `Q` 또는 `Esc`: 종료

## 폴더 구조

```text
computer-graphics/
├─ 컴그/
│  ├─ 컴그.slnx
│  ├─ PortableOpenGL.props
│  └─ 컴그/
│     ├─ 1-9.cpp
│     ├─ 1-10.cpp
│     ├─ 1-11.cpp
│     ├─ 1-12.cpp
│     ├─ Simple2D.h
│     └─ Practice*.vcxproj
└─ third_party/
   ├─ include/GL
   ├─ lib/x64
   └─ bin/x64
```

`PortableOpenGL.props`가 저장소 안의 OpenGL 의존성 위치를 모든 프로젝트에 자동으로 연결하고, 빌드 후 `glew32.dll`을 실행 파일 폴더로 복사합니다.
