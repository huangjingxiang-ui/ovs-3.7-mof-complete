# 如果没有 curl，先安装（Debian/Ubuntu）
apt-get update && apt-get install -y curl

# 安装 uv
curl -LsSf https://astral.sh/uv/install.sh | sh

# 刷新环境变量（让 uv 命令立即生效）
source $HOME/.local/bin/env



# 1. 进入项目目录

# 2. 创建虚拟环境（默认生成 .venv 文件夹）
uv venv

# 3. 激活环境（可选，但推荐）
source .venv/bin/activate

# 4. 如果有 requirements.txt，一键安装所有依赖
uv pip install -r requirements.txt
