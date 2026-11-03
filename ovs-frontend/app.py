from flask import Flask, render_template, request, jsonify, session
import yaml
import subprocess
import os
import requests
from pathlib import Path

app = Flask(__name__)
app.secret_key = 'your-strong-secret-key'  # 用于session加密，生产环境请更换

# --- 数据加载 ---

def load_main_protocols():
    """加载主协议配置文件，并确保所有值都是列表"""
    # 获取脚本所在目录的父目录，然后构建配置文件路径
    current_dir = Path(__file__).parent.resolve()  # 变为绝对路径
    config_path = current_dir.parent / 'Protocol_cfg' / 'header_describe' / 'main.yml'
    with open(config_path, 'r', encoding='utf-8') as file:
        data = yaml.safe_load(file)
    
    # 确保每个层级的值都是一个列表，防止 'NoneType' is not iterable 错误
    if isinstance(data, dict):
        for key, value in data.items():
            if value is None:
                data[key] = []
    return data

def load_protocol_fields(protocol_name):
    """加载单个协议的字段定义，并保持文件中的顺序"""
    # 使用与load_main_protocols相同的路径构建方式
    current_dir = Path(__file__).parent.resolve()
    config_path = current_dir.parent / 'Protocol_cfg' / 'header_describe' / f'{protocol_name}.yml'
    
    if not config_path.exists():
        return []  # 返回空列表
    try:
        with open(config_path, 'r', encoding='utf-8') as file:
            # PyYAML 5.1+ 在 Python 3.7+ 上会默认保留顺序
            config = yaml.safe_load(file)
            if config and isinstance(config, dict) and protocol_name in config:
                fields_dict = config[protocol_name]
                if not isinstance(fields_dict, dict):
                    return []
                
                # 将字典转换为字典列表以保证顺序
                ordered_fields = []
                for field_name, field_config in fields_dict.items():
                    if isinstance(field_config, dict) and 'size' in field_config:
                        ordered_fields.append({
                            'name': field_name,
                            'config': field_config
                        })
                return ordered_fields
    except Exception as e:
        print(f"加载协议 {protocol_name} 错误: {e}")
    return [] # 返回空列表

def load_composite_actions():
    """加载已保存的复合动作文件，按创建时间从新到旧排序"""
    import os
    from datetime import datetime
    
    # 使用与其他配置文件相同的路径构建方式
    current_dir = Path(__file__).parent.resolve()
    composite_dir = current_dir / 'compound_actions'
    # print(f"查找复合动作目录: {composite_dir.resolve()}")
    
    if not composite_dir.exists():
        print("复合动作目录不存在，创建目录")
        composite_dir.mkdir(parents=True, exist_ok=True)
        return []
    
    loaded_actions = []
    yml_files = list(composite_dir.glob('*.yml'))
    # print(f"找到 {len(yml_files)} 个yml文件: {[f.name for f in yml_files]}")
    
    for yml_file in yml_files:
        try:
            with open(yml_file, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)
                if data and 'name' in data and 'actions' in data:
                    # 获取文件的创建时间（修改时间）
                    file_stat = yml_file.stat()
                    created_time = datetime.fromtimestamp(file_stat.st_mtime)
                    
                    # 添加创建时间信息到数据中
                    data['created_time'] = created_time.strftime('%Y-%m-%d %H:%M:%S')
                    data['created_timestamp'] = file_stat.st_mtime
                    
                    loaded_actions.append(data)
                    # print(f"成功加载复合动作: {data.get('name')}")
                else:
                    print(f"文件 {yml_file.name} 格式不正确: {data}")
        except Exception as e:
            print(f"加载复合动作文件 {yml_file} 失败: {e}")
    
    # 按创建时间排序，从新到旧（降序）
    loaded_actions.sort(key=lambda x: x.get('created_timestamp', 0), reverse=True)
    
    # print(f"总共加载了 {len(loaded_actions)} 个复合动作")
    return loaded_actions

# --- 配置定义 ---

# 原子动作
ATOMIC_ACTIONS = {
    'add_protocol': {'description': '添加协议头'},
    'del_protocol': {'description': '删除协议头'},
    'mod_fields': {'description': '修改协议字段'},
    'output': {'description': '输出到端口'},
    # 'clone': {'description': '克隆数据包并执行嵌套动作'}  # 暂时不提供clone动作选择
}

# 加载协议
try:
    protocols = load_main_protocols()
    # print(f"成功加载协议配置: {list(protocols.keys()) if isinstance(protocols, dict) else '非字典类型'}")
except Exception as e:
    # print(f"加载协议配置失败: {e}")
    protocols = {}

# --- API 路由 ---

@app.route('/')
def index():
    """主页"""
    saved_composite_actions = load_composite_actions()
    return render_template('index.html', 
                         atomic_actions=ATOMIC_ACTIONS, 
                         protocols=protocols,
                         composite_actions=saved_composite_actions)

@app.route('/api/protocols')
def get_protocols():
    """获取协议列表API"""
    return jsonify(load_main_protocols())

@app.route('/api/protocol-fields/<protocol_name>')
def get_protocol_fields(protocol_name):
    """获取协议字段API"""
    return jsonify(load_protocol_fields(protocol_name))

@app.route('/api/composite-actions')
def get_composite_actions():
    """获取已保存的复合动作列表API"""
    return jsonify(load_composite_actions())

@app.route('/api/composite-actions/check-name/<action_name>')
def check_action_name_exists(action_name):
    """检查复合动作名称是否已存在"""
    current_dir = Path(__file__).parent.resolve()
    composite_dir = current_dir / 'compound_actions'
    file_path = composite_dir / f"{action_name}.yml"
    
    exists = file_path.exists()
    return jsonify({
        "exists": exists,
        "name": action_name,
        "message": f"动作名称 '{action_name}' {'已存在' if exists else '可用'}"
    })

@app.route('/api/composite-actions/<action_name>')
def get_composite_action_detail(action_name):
    """获取单个复合动作的详细信息"""
    from datetime import datetime
    
    current_dir = Path(__file__).parent.resolve()
    composite_dir = current_dir / 'compound_actions'
    file_path = composite_dir / f"{action_name}.yml"
    
    if not file_path.exists():
        return jsonify({"error": "复合动作不存在"}), 404
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            data = yaml.safe_load(f)
            
            # 添加文件创建时间信息
            file_stat = file_path.stat()
            created_time = datetime.fromtimestamp(file_stat.st_mtime)
            data['created_time'] = created_time.strftime('%Y-%m-%d %H:%M:%S')
            data['created_timestamp'] = file_stat.st_mtime
            
            return jsonify(data)
    except Exception as e:
        return jsonify({"error": f"读取文件失败: {str(e)}"}), 500

@app.route('/api/save', methods=['POST'])
def save_actions():
    """保存复合动作序列到文件"""
    data = request.json
    actions = data.get('actions', [])
    name = data.get('name', 'unnamed_action')
    force_overwrite = data.get('force_overwrite', False)  # 是否强制覆盖
    
    # 定义保存路径 - 使用应用目录下的compound_actions
    current_dir = Path(__file__).parent.resolve()
    save_dir = current_dir / 'compound_actions'
    save_dir.mkdir(parents=True, exist_ok=True)
    
    # 使用 .yml 扩展名
    file_path = save_dir / f"{name}.yml"
    
    # 检查文件是否已存在（除非强制覆盖）
    if file_path.exists() and not force_overwrite:
        return jsonify({
            "status": "name_exists", 
            "message": f"复合动作名称 '{name}' 已存在，请修改动作名或选择覆盖",
            "existing_file": str(file_path)
        }), 409  # 409 Conflict 状态码表示冲突
    
    try:
        # 创建有序的数据结构来保持期望的字段顺序
        ordered_actions = []
        for action in actions:
            ordered_action = {}
            # 按期望的顺序添加字段
            if 'type' in action:
                ordered_action['type'] = action['type']
            if 'protocol' in action:
                ordered_action['protocol'] = action['protocol']
            if 'params' in action:
                ordered_action['params'] = action['params']
            # 处理output动作的特殊字段
            if 'parameter' in action:
                ordered_action['parameter'] = action['parameter']
            # 处理clone动作的特殊字段
            if 'nested_actions' in action:
                ordered_action['nested_actions'] = action['nested_actions']
            ordered_actions.append(ordered_action)
        
        # 创建最终的有序数据结构
        ordered_data = {
            'name': name,
            'actions': ordered_actions
        }
        
        with open(file_path, 'w', encoding='utf-8') as f:
            yaml.dump(ordered_data, f, allow_unicode=True, sort_keys=False, default_flow_style=False)
        return jsonify({"status": "success", "message": f"成功保存动作 '{name}' 到 {file_path}"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

@app.route('/api/composite-actions/<action_name>', methods=['DELETE'])
def delete_composite_action(action_name):
    """删除复合动作文件"""
    # 定义复合动作文件路径
    current_dir = Path(__file__).parent.resolve()
    composite_dir = current_dir / 'compound_actions'
    file_path = composite_dir / f"{action_name}.yml"
    
    try:
        # 检查文件是否存在
        if not file_path.exists():
            return jsonify({"status": "error", "message": f"复合动作 '{action_name}' 不存在"}), 404
        
        # 删除文件
        file_path.unlink()
        return jsonify({"status": "success", "message": f"成功删除复合动作 '{action_name}'"})
    except Exception as e:
        return jsonify({"status": "error", "message": f"删除失败: {str(e)}"}), 500

@app.route('/api/bridges', methods=['POST'])
def get_bridges():
    """获取网桥列表"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 首先尝试直接执行（适用于容器环境或有权限的用户）
        try:
            result = subprocess.run(['ovs-vsctl', 'list-br'], capture_output=True, text=True, timeout=10)
            
            # 如果直接执行成功，处理结果
            if result.returncode == 0:
                bridges = [bridge.strip() for bridge in result.stdout.split('\n') if bridge.strip()]
                return jsonify({"status": "success", "bridges": bridges})
            else:
                print(f"直接执行ovs-vsctl list-br失败: {result.stderr}")
        except FileNotFoundError:
            print("ovs-vsctl命令未找到")
        except Exception as e:
            print(f"直接执行ovs-vsctl list-br时发生错误: {e}")
        
        # 如果直接执行失败，检查是否需要sudo权限
        if check_sudo_required_for_vsctl():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            # 使用echo传递密码给sudo
            command = f'echo "{sudo_password}" | sudo -S ovs-vsctl list-br'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=10)
            
            # 检查密码是否正确
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                # 密码错误时清除session缓存
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                # 密码正确，缓存到session
                session['sudo_password'] = sudo_password
                bridges = [bridge.strip() for bridge in result.stdout.split('\n') if bridge.strip()]
                return jsonify({"status": "success", "bridges": bridges})
        
        # 如果所有尝试都失败了
        return jsonify({"status": "error", "message": "无法执行ovs-vsctl命令"}), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "获取网桥列表超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": "获取网桥列表时发生错误", "error": str(e)}), 500

@app.route('/api/ovs-interface-states', methods=['POST'])
def get_ovs_interface_states_api():
    """获取OVS接口状态信息的API端点"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        interfaces = get_ovs_interface_states(sudo_password)
        
        return jsonify({
            "status": "success",
            "interfaces": interfaces,
            "count": len(interfaces),
            "message": f"成功获取 {len(interfaces)} 个接口的状态信息"
        })
        
    except Exception as e:
        return jsonify({
            "status": "error", 
            "message": f"获取OVS接口状态时发生错误: {str(e)}"
        }), 500

@app.route('/api/flows/<bridge>', methods=['POST'])
def get_flows(bridge):
    """获取指定网桥的流表信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 首先获取端口名映射
        port_mapping = get_port_name_mapping(bridge, sudo_password)
        
        # 检查是否需要sudo权限
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            # 使用echo传递密码给sudo
            command = f'echo "{sudo_password}" | sudo -S ovs-ofctl dump-flows-mof {bridge}'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=15)
            
            # 检查密码是否正确
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                # 密码错误时清除session缓存
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                # 密码正确，缓存到session
                session['sudo_password'] = sudo_password
        else:
            result = subprocess.run(['ovs-ofctl', 'dump-flows-mof', bridge], 
                                  capture_output=True, text=True, timeout=15)
        
        if result.returncode == 0:
            flows = parse_flow_dump(result.stdout, port_mapping)
            return jsonify({"status": "success", "flows": flows})
        else:
            return jsonify({"status": "error", "message": "获取流表失败", "error": result.stderr}), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "获取流表超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": "获取流表时发生错误", "error": str(e)}), 500









@app.route('/api/container-bridges/<container_ip>', methods=['POST'])
def get_container_bridges(container_ip):
    """通过IP获取容器的网桥信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 10)
    
    try:
        # 构建容器API URL
        container_url = f"http://{container_ip}:5000/api/bridges"
        
        # 准备请求数据
        request_data = {}
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        # 发送请求到容器
        response = requests.post(
            container_url, 
            json=request_data, 
            timeout=timeout
        )
        
        if response.status_code == 200:
            container_data = response.json()
            return jsonify({
                "status": "success",
                "container_ip": container_ip,
                "bridges": container_data.get('bridges', []),
                "message": f"成功获取容器 {container_ip} 的网桥信息"
            })
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时，请检查容器IP和端口是否正确"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error", 
            "container_ip": container_ip,
            "message": "无法连接到容器，请检查容器是否运行且端口开放"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"获取容器网桥信息失败: {str(e)}"
        }), 500

@app.route('/api/container-flows/<container_ip>/<bridge>', methods=['POST'])
def get_container_flows(container_ip, bridge):
    """通过IP获取容器指定网桥的流表信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 15)
    
    try:
        # 构建容器API URL
        container_url = f"http://{container_ip}:5000/api/flows/{bridge}"
        
        # 准备请求数据
        request_data = {}
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        # 发送请求到容器
        response = requests.post(
            container_url,
            json=request_data,
            timeout=timeout
        )
        
        if response.status_code == 200:
            container_data = response.json()
            # 容器的流表数据应该已经包含端口名转换
            return jsonify({
                "status": "success",
                "container_ip": container_ip,
                "bridge": bridge,
                "flows": container_data.get('flows', []),
                "message": f"成功获取容器 {container_ip} 网桥 {bridge} 的流表信息"
            })
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "bridge": bridge,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip, 
            "message": "无法连接到容器"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"获取容器流表信息失败: {str(e)}"
        }), 500

@app.route('/api/port-statistics/<bridge>', methods=['POST'])
def get_port_statistics(bridge):
    """获取本地网桥的端口统计信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 检查是否需要sudo权限
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            # 使用echo传递密码给sudo
            command = f'echo "{sudo_password}" | sudo -S ovs-ofctl dump-ports {bridge}'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=15)
            
            # 检查密码是否正确
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                session['sudo_password'] = sudo_password
        else:
            result = subprocess.run(['ovs-ofctl', 'dump-ports', bridge], 
                                  capture_output=True, text=True, timeout=15)
        
        if result.returncode == 0:
            port_stats = parse_port_statistics(result.stdout)
            return jsonify({
                "status": "success", 
                "bridge": bridge,
                "port_statistics": port_stats
            })
        else:
            return jsonify({
                "status": "error", 
                "message": "获取端口统计失败", 
                "error": result.stderr
            }), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "获取端口统计超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": f"获取端口统计时发生错误: {str(e)}"}), 500

@app.route('/api/local-bridge-ports/<bridge>', methods=['POST'])
def get_local_bridge_ports(bridge):
    """获取本机网桥的端口信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 使用ovs-ofctl show命令获取端口信息
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            command = f'echo "{sudo_password}" | sudo -S ovs-ofctl show {bridge}'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=15)
            
            # 检查密码是否正确
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                session['sudo_password'] = sudo_password
        else:
            result = subprocess.run(['ovs-ofctl', 'show', bridge], 
                                  capture_output=True, text=True, timeout=15)
        
        if result.returncode == 0:
            ports = parse_ovs_show_output(result.stdout, sudo_password)
            return jsonify({
                "status": "success",
                "bridge": bridge,
                "ports": ports,
                "message": f"成功获取本机网桥 {bridge} 的端口信息"
            })
        else:
            return jsonify({
                "status": "error",
                "bridge": bridge,
                "message": "获取端口信息失败",
                "error": result.stderr
            }), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "获取端口信息超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": f"获取端口信息时发生错误: {str(e)}"}), 500

@app.route('/api/container-bridge-ports/<container_ip>/<bridge>', methods=['POST'])
def get_container_bridge_ports(container_ip, bridge):
    """通过IP获取容器网桥的端口信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 15)
    
    try:
        # 直接调用容器的端口信息API（而不是流表API）
        container_url = f"http://{container_ip}:5000/api/local-bridge-ports/{bridge}"
        request_data = {}
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        response = requests.post(container_url, json=request_data, timeout=timeout)
        
        if response.status_code == 200:
            # 直接使用容器返回的端口信息
            container_data = response.json()
            if container_data.get('status') == 'success':
                return jsonify({
                    "status": "success",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "ports": container_data.get('ports', []),
                    "message": f"成功获取容器 {container_ip} 网桥 {bridge} 的端口信息"
                })
            else:
                return jsonify({
                    "status": "error",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "message": container_data.get('message', '获取端口信息失败'),
                    "error": container_data.get('error', '')
                }), 500
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "bridge": bridge,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "无法连接到容器"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"获取容器端口信息失败: {str(e)}"
        }), 500

@app.route('/api/container-port-statistics/<container_ip>/<bridge>', methods=['POST'])
def get_container_port_statistics(container_ip, bridge):
    """通过IP获取容器网桥的端口统计信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 15)
    
    try:
        # 调用容器的端口统计API
        container_url = f"http://{container_ip}:5000/api/port-statistics/{bridge}"
        request_data = {}
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        response = requests.post(container_url, json=request_data, timeout=timeout)
        
        if response.status_code == 200:
            container_data = response.json()
            if container_data.get('status') == 'success':
                return jsonify({
                    "status": "success",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "port_statistics": container_data.get('port_statistics', []),
                    "message": f"成功获取容器 {container_ip} 网桥 {bridge} 的端口统计信息"
                })
            else:
                return jsonify({
                    "status": "error",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "message": container_data.get('message', '获取端口统计信息失败'),
                    "error": container_data.get('error', '')
                }), 500
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "bridge": bridge,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "无法连接到容器"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"获取容器端口统计失败: {str(e)}"
        }), 500

@app.route('/api/flow-based-port-statistics/<bridge>', methods=['POST'])
def get_flow_based_port_statistics(bridge):
    """基于流表信息统计端口的输出数据包数"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 首先获取端口名映射
        port_mapping = get_port_name_mapping(bridge, sudo_password)
        
        # 获取流表信息
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            command = f'echo "{sudo_password}" | sudo -S ovs-ofctl dump-flows-mof {bridge}'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=15)
            
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                session['sudo_password'] = sudo_password
        else:
            result = subprocess.run(['ovs-ofctl', 'dump-flows-mof', bridge], 
                                  capture_output=True, text=True, timeout=15)
        
        if result.returncode != 0:
            return jsonify({
                "status": "error", 
                "message": "获取流表失败", 
                "error": result.stderr
            }), 500
        
        # 解析流表并统计每个端口的输出数据包数
        flows = parse_flow_dump(result.stdout, port_mapping)
        port_output_stats = calculate_port_output_from_flows(flows, port_mapping)
        
        return jsonify({
            "status": "success",
            "bridge": bridge,
            "port_output_statistics": port_output_stats,
            "message": f"成功基于流表统计网桥 {bridge} 的端口输出数据包数"
        })
        
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "获取流表统计超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": f"获取流表统计时发生错误: {str(e)}"}), 500

@app.route('/api/container-flow-based-port-statistics/<container_ip>/<bridge>', methods=['POST'])
def get_container_flow_based_port_statistics(container_ip, bridge):
    """通过IP获取容器网桥基于流表的端口统计信息"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 15)
    
    try:
        # 调用容器的基于流表的端口统计API
        container_url = f"http://{container_ip}:5000/api/flow-based-port-statistics/{bridge}"
        request_data = {}
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        response = requests.post(container_url, json=request_data, timeout=timeout)
        
        if response.status_code == 200:
            container_data = response.json()
            if container_data.get('status') == 'success':
                return jsonify({
                    "status": "success",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "port_output_statistics": container_data.get('port_output_statistics', []),
                    "message": f"成功获取容器 {container_ip} 网桥 {bridge} 基于流表的端口统计信息"
                })
            else:
                return jsonify({
                    "status": "error",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "message": container_data.get('message', '获取基于流表的端口统计信息失败'),
                    "error": container_data.get('error', '')
                }), 500
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "bridge": bridge,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "无法连接到容器"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"获取容器基于流表的端口统计失败: {str(e)}"
        }), 500

@app.route('/api/set-port-down', methods=['POST'])
def set_port_down():
    """设置本机端口为down状态"""
    data = request.json or {}
    bridge = data.get('bridge', '')
    port = data.get('port', '')
    sudo_password = data.get('sudo_password', '')
    
    if not bridge or not port:
        return jsonify({
            "status": "error",
            "message": "缺少必要参数: bridge和port"
        }), 400
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 检查是否需要sudo权限
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            # 优先使用ip link set命令，这是最直接有效的方法
            command = f'echo "{sudo_password}" | sudo -S ip link set {port} down'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=10)
            used_method = "IP链路关闭"
            
            # 检查密码是否正确
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                session['sudo_password'] = sudo_password
        else:
            # 非sudo模式：尝试直接使用ip link set命令
            result = subprocess.run(['ip', 'link', 'set', port, 'down'], 
                                  capture_output=True, text=True, timeout=10)
            used_method = "IP链路关闭"
            
            # 如果权限不足，要求sudo密码
            if result.returncode != 0 and ("Operation not permitted" in result.stderr or "Permission denied" in result.stderr):
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo权限执行ip link set命令，请输入sudo密码"
                })
        
        if result.returncode == 0:
            return jsonify({
                "status": "success",
                "message": f"成功设置端口 {port} 为down状态 (使用方法: {used_method})",
                "bridge": bridge,
                "port": port,
                "method": used_method
            })
        else:
            return jsonify({
                "status": "error", 
                "message": f"设置端口down失败: {result.stderr}",
                "bridge": bridge,
                "port": port
            }), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "设置端口down超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": f"设置端口down时发生错误: {str(e)}"}), 500

@app.route('/api/set-port-up', methods=['POST'])
def set_port_up():
    """设置本机端口为up状态（恢复链路）"""
    data = request.json or {}
    bridge = data.get('bridge', '')
    port = data.get('port', '')
    sudo_password = data.get('sudo_password', '')
    
    if not bridge or not port:
        return jsonify({
            "status": "error",
            "message": "缺少必要参数: bridge和port"
        }), 400
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 检查是否需要sudo权限
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            # 优先使用ip link set命令恢复端口
            command = f'echo "{sudo_password}" | sudo -S ip link set {port} up'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=10)
            used_method = "IP链路启用"
            
            # 检查密码是否正确
            if "Sorry, try again" in result.stderr or "incorrect password" in result.stderr.lower():
                session.pop('sudo_password', None)
                return jsonify({
                    "status": "password_incorrect",
                    "message": "sudo密码错误，请重新输入"
                }), 401
            elif result.returncode == 0:
                session['sudo_password'] = sudo_password
        else:
            # 非sudo模式：尝试直接使用ip link set命令
            result = subprocess.run(['ip', 'link', 'set', port, 'up'], 
                                  capture_output=True, text=True, timeout=10)
            used_method = "IP链路启用"
            
            # 如果权限不足，要求sudo密码
            if result.returncode != 0 and ("Operation not permitted" in result.stderr or "Permission denied" in result.stderr):
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo权限执行ip link set命令，请输入sudo密码"
                })
        
        if result.returncode == 0:
            return jsonify({
                "status": "success",
                "message": f"成功设置端口 {port} 为up状态 (使用方法: {used_method})",
                "bridge": bridge,
                "port": port,
                "method": used_method
            })
        else:
            return jsonify({
                "status": "error", 
                "message": f"设置端口up失败: {result.stderr}",
                "bridge": bridge,
                "port": port
            }), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({"status": "error", "message": "设置端口up超时"}), 500
    except Exception as e:
        return jsonify({"status": "error", "message": f"设置端口up时发生错误: {str(e)}"}), 500

@app.route('/api/container-set-port-down/<container_ip>', methods=['POST'])
def container_set_port_down(container_ip):
    """设置容器端口为down状态"""
    data = request.json or {}
    bridge = data.get('bridge', '')
    port = data.get('port', '')
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 15)
    
    if not bridge or not port:
        return jsonify({
            "status": "error",
            "message": "缺少必要参数: bridge和port"
        }), 400
    
    try:
        # 调用容器的设置端口down API
        container_url = f"http://{container_ip}:5000/api/set-port-down"
        request_data = {
            'bridge': bridge,
            'port': port
        }
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        response = requests.post(container_url, json=request_data, timeout=timeout)
        
        if response.status_code == 200:
            container_data = response.json()
            if container_data.get('status') == 'success':
                return jsonify({
                    "status": "success",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "port": port,
                    "message": f"成功设置容器 {container_ip} 端口 {port} 为down状态"
                })
            else:
                return jsonify({
                    "status": "error",
                    "container_ip": container_ip,
                    "message": container_data.get('message', '设置端口down失败'),
                    "error": container_data.get('error', '')
                }), 500
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "无法连接到容器"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"设置容器端口down失败: {str(e)}"
        }), 500

@app.route('/api/container-set-port-up/<container_ip>', methods=['POST'])
def container_set_port_up(container_ip):
    """设置容器端口为up状态（恢复链路）"""
    data = request.json or {}
    bridge = data.get('bridge', '')
    port = data.get('port', '')
    sudo_password = data.get('sudo_password', '')
    timeout = data.get('timeout', 15)
    
    if not bridge or not port:
        return jsonify({
            "status": "error",
            "message": "缺少必要参数: bridge和port"
        }), 400
    
    try:
        # 调用容器的设置端口up API
        container_url = f"http://{container_ip}:5000/api/set-port-up"
        request_data = {
            'bridge': bridge,
            'port': port
        }
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        response = requests.post(container_url, json=request_data, timeout=timeout)
        
        if response.status_code == 200:
            container_data = response.json()
            if container_data.get('status') == 'success':
                return jsonify({
                    "status": "success",
                    "container_ip": container_ip,
                    "bridge": bridge,
                    "port": port,
                    "message": f"成功设置容器 {container_ip} 端口 {port} 为up状态",
                    "method": container_data.get('method', '未知方法')
                })
            else:
                return jsonify({
                    "status": "error",
                    "container_ip": container_ip,
                    "message": container_data.get('message', '设置端口up失败'),
                    "error": container_data.get('error', '')
                }), 500
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "message": f"容器返回错误: {response.status_code}",
                "error": response.text
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "无法连接到容器"
        }), 503
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"设置容器端口up失败: {str(e)}"
        }), 500

@app.route('/api/test-container-connection/<container_ip>', methods=['POST'])
def test_container_connection(container_ip):
    """测试容器连接"""
    try:
        # 测试连接到容器的协议API（最简单的GET接口）
        container_url = f"http://{container_ip}:5000/api/protocols"
        
        response = requests.get(container_url, timeout=5)
        
        if response.status_code == 200:
            return jsonify({
                "status": "success",
                "container_ip": container_ip,
                "message": "容器连接成功，OVS管理接口正常",
                "response_time": "< 5s"
            })
        else:
            return jsonify({
                "status": "error",
                "container_ip": container_ip,
                "message": f"容器响应异常，状态码: {response.status_code}"
            }), response.status_code
            
    except requests.exceptions.ConnectTimeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "连接容器超时，请检查容器IP和网络连接"
        }), 504
    except requests.exceptions.ConnectionError:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "无法连接到容器，请检查容器是否运行且端口开放"
        }), 503
    except requests.exceptions.Timeout:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": "请求容器超时"
        }), 504
    except Exception as e:
        return jsonify({
            "status": "error",
            "container_ip": container_ip,
            "message": f"测试连接时发生错误: {str(e)}"
        }), 500

@app.route('/anomaly-detection')
def anomaly_detection():
    """异常检测与恢复页面"""
    return render_template('anomaly_detection.html')

@app.route('/api/test-flow-statistics/<bridge>', methods=['POST'])
def test_flow_statistics(bridge):
    """测试流表统计功能"""
    data = request.json or {}
    sudo_password = data.get('sudo_password', '')
    
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    try:
        # 获取原始流表
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令"
                })
            command = f'echo "{sudo_password}" | sudo -S ovs-ofctl dump-flows-mof {bridge}'
            result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=15)
        else:
            result = subprocess.run(['ovs-ofctl', 'dump-flows-mof', bridge], 
                                  capture_output=True, text=True, timeout=15)
        
        if result.returncode != 0:
            return jsonify({
                "status": "error",
                "message": "获取流表失败",
                "error": result.stderr
            }), 500
        
        # 获取端口映射
        port_mapping = get_port_name_mapping(bridge, sudo_password)
        
        # 解析流表
        flows = parse_flow_dump(result.stdout, port_mapping)
        
        # 计算基于流表的端口统计
        port_output_stats = calculate_port_output_from_flows(flows, port_mapping)
        
        return jsonify({
            "status": "success",
            "bridge": bridge,
            "raw_flows_count": len(flows),
            "port_mapping": port_mapping,
            "flows_summary": [
                {
                    "match": flow.get('match', ''),
                    "actions": flow.get('actions', ''),
                    "packet_count": flow.get('packet_count', 0),
                    "byte_count": flow.get('byte_count', 0)
                } for flow in flows[:5]  # 只显示前5条流表作为示例
            ],
            "port_output_statistics": port_output_stats,
            "message": f"成功解析网桥 {bridge} 的流表统计信息"
        })
        
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": f"测试流表统计时发生错误: {str(e)}"
        }), 500

@app.route('/api/deploy-flow', methods=['POST'])
def deploy_flow():
    """下发流表到OVS交换机"""
    data = request.json
    bridge = data.get('bridge', 'br0')
    command_type = data.get('command_type', 'add-flow-mof')
    match_conditions = data.get('match_conditions', '')
    actions_list = data.get('actions', [])
    sudo_password = data.get('sudo_password', '')  # 获取sudo密码
    # 优先使用session缓存的密码
    if not sudo_password and 'sudo_password' in session:
        sudo_password = session['sudo_password']
    
    print(actions_list)
    # 构建动作字符串
    action_parts = []
    
    # 处理动作配置中的所有动作
    for action_data in actions_list:
        action_type = action_data.get('type')
        
        if action_type == 'custom':
            # 处理自定义动作
            custom_action = action_data.get('custom_action', '')
            if custom_action.strip():
                action_parts.append(custom_action.strip())
        elif action_type == 'composite':
            # 处理复合动作 - 展开为子动作
            sub_actions = action_data.get('actions', [])
            for sub_action in sub_actions:
                sub_action_type = sub_action.get('type')
                if sub_action_type == 'output':
                    parameter = sub_action.get('parameter', '')
                    action_parts.append(f'output:{parameter}')
                elif sub_action_type == 'clone':
                    action_str = build_action_string(sub_action)
                    action_parts.append(action_str)
                elif sub_action_type in ['add_protocol', 'del_protocol', 'mod_fields']:
                    action_str = build_action_string(sub_action)
                    action_parts.append(action_str)
        elif action_type == 'clone':
            # 处理clone动作
            action_str = build_action_string(action_data)
            action_parts.append(action_str)
        elif action_type == 'output':
            # 处理output动作
            parameter = action_data.get('parameter', '')
            action_parts.append(f'output:{parameter}')
        elif action_type in ['add_protocol', 'del_protocol', 'mod_fields']:
            # 处理原子动作
            action_str = build_action_string(action_data)
            action_parts.append(action_str)
    
    if not action_parts:
        return jsonify({"status": "error", "message": "至少需要在动作配置中添加一个动作"}), 400
    
    # 构建完整的流表命令
    actions_str = ','.join(action_parts)
    
    if match_conditions.strip():
        flow_rule = f'"{match_conditions.strip()},actions={actions_str}"'
    else:
        flow_rule = f'"actions={actions_str}"'
    
    ovs_prefix = get_ovs_command_prefix()
    command = f'{ovs_prefix} {command_type} {bridge} {flow_rule}'
    
    try:
        # 检查是否需要sudo权限
        if check_sudo_required():
            if not sudo_password:
                return jsonify({
                    "status": "password_required",
                    "message": "需要sudo密码才能执行此命令",
                    "command": command
                })
            # 使用echo传递密码给sudo
            ovs_command_only = command.replace("sudo ", "")
            full_command = f'echo "{sudo_password}" | sudo -S {ovs_command_only}'
            result = subprocess.run(full_command, shell=True, capture_output=True, text=True, check=True)
            # 如果密码正确，缓存到session
            session['sudo_password'] = sudo_password
        else:
            result = subprocess.run(command, shell=True, capture_output=True, text=True, check=True)
        return jsonify({
            "status": "success",
            "message": "流表下发成功",
            "command": command,
            "output": result.stdout
        })
    except subprocess.CalledProcessError as e:
        error_msg = e.stderr
        # 检查是否是密码错误
        if "Sorry, try again" in error_msg or "incorrect password" in error_msg.lower() or "authentication failure" in error_msg.lower():
            # 密码错误时清除session缓存
            session.pop('sudo_password', None)
            return jsonify({
                "status": "password_incorrect",
                "message": "sudo密码错误，请重新输入",
                "command": command,
                "error": "密码验证失败"
            }), 401
        else:
            return jsonify({
                "status": "error",
                "message": "流表下发失败",
                "command": command,
                "error": error_msg
            }), 500
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": "执行命令时发生错误",
            "command": command,
            "error": str(e)
        }), 500

# --- 辅助函数 ---

def check_sudo_required():
    """检测是否需要sudo权限执行ovs-ofctl命令"""
    try:
        # 首先检查ovs-ofctl命令是否存在
        result = subprocess.run(['ovs-ofctl', '--version'], 
                              capture_output=True, text=True, timeout=5)
        if result.returncode != 0:
            return True
        
        # 尝试列出所有交换机来测试权限
        test_result = subprocess.run(['ovs-vsctl', 'list-br'], 
                                   capture_output=True, text=True, timeout=5)
        if test_result.returncode == 0:
            # 如果可以列出交换机，说明有基本权限，通常不需要sudo
            return False
        else:
            # 如果不能列出交换机，可能需要sudo权限
            return True
    except (subprocess.TimeoutExpired, FileNotFoundError, Exception):
        # 如果命令不存在或执行失败，默认使用sudo
        return True

def check_sudo_required_for_vsctl():
    """检测是否需要sudo权限执行ovs-vsctl命令"""
    try:
        # 尝试执行ovs-vsctl list-br命令测试权限
        test_result = subprocess.run(['ovs-vsctl', 'list-br'], 
                                   capture_output=True, text=True, timeout=5)
        if test_result.returncode == 0:
            # 如果可以列出交换机，说明不需要sudo权限
            return False
        else:
            # 如果不能列出交换机，可能需要sudo权限
            return True
    except (subprocess.TimeoutExpired, FileNotFoundError, Exception):
        # 如果命令不存在或执行失败，默认使用sudo
        return True

def get_ovs_command_prefix():
    """获取OVS命令前缀（是否需要sudo）"""
    if check_sudo_required():
        return 'sudo ovs-ofctl'
    else:
        return 'ovs-ofctl'

def get_ovs_command_prefix_simple():
    """获取简单的OVS命令前缀（不需要sudo的版本）"""
    return 'ovs-ofctl'

def get_port_name_mapping(bridge, sudo_password=None):
    """获取端口号到端口名的映射"""
    try:
        # 使用ovs-ofctl show获取端口信息
        if check_sudo_required():
            if sudo_password:
                command = f'echo "{sudo_password}" | sudo -S ovs-ofctl show {bridge}'
                result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=10)
            else:
                return {}
        else:
            result = subprocess.run(['ovs-ofctl', 'show', bridge], 
                                  capture_output=True, text=True, timeout=10)
        
        if result.returncode != 0:
            return {}
        
        port_mapping = {}
        lines = result.stdout.strip().split('\n')
        
        for line in lines:
            line = line.strip()
            if not line:
                continue
                
            # 解析端口行，格式: 1(vhost-user1): addr:02:42:ac:11:00:02
            import re
            port_match = re.match(r'(\d+)\(([^)]+)\):', line)
            if port_match:
                port_number = port_match.group(1)
                port_name = port_match.group(2)
                port_mapping[port_number] = port_name
        
        return port_mapping
        
    except Exception as e:
        print(f"获取端口映射失败: {e}")
        return {}

def convert_port_numbers_to_names(text, port_mapping):
    """将文本中的端口号转换为端口名"""
    if not port_mapping or not text:
        return text
    
    import re
    
    # 处理 in_port=数字 的情况
    def replace_in_port(match):
        port_num = match.group(1)
        if port_num in port_mapping:
            return f'in_port={port_mapping[port_num]}'
        return match.group(0)
    
    text = re.sub(r'in_port=(\d+)', replace_in_port, text)
    
    # 处理 output:数字 的情况
    def replace_output_port(match):
        port_num = match.group(1)
        if port_num in port_mapping:
            return f'output:{port_mapping[port_num]}'
        return match.group(0)
    
    text = re.sub(r'output:(\d+)', replace_output_port, text)
    
    return text

def parse_flow_dump(output, port_mapping=None):
    """解析流表dump输出（支持MOF格式）"""
    if port_mapping is None:
        port_mapping = {}
        
    flows = []
    lines = output.strip().split('\n')
    
    print(f"流表输出调试: 共{len(lines)}行")  # 调试信息
    for i, line in enumerate(lines):
        print(f"第{i+1}行: '{line}'")  # 调试信息
    
    for line in lines:
        original_line = line
        line = line.strip()
        
        # 跳过空行和头部信息
        if not line or line.startswith('NXST_FLOW') or line.startswith('OFPST_FLOW'):
            print(f"跳过头部行: {line}")
            continue
            
        # 跳过只包含cookie=的行（这可能是头部信息）
        if line.startswith('cookie=') and ' ' not in line:
            print(f"跳过纯cookie行: {line}")
            continue
        
        # 检查是否是有效的流表行（必须包含cookie和actions）
        if 'cookie=' not in line or 'actions=' not in line:
            print(f"跳过无效流表行: {line}")
            continue
            
        try:
            # 解析流表行
            flow_info = {}
            
            # 设置默认值
            flow_info['cookie'] = 'unknown'
            flow_info['priority'] = 32768
            flow_info['match'] = 'any'
            flow_info['actions'] = '-'
            flow_info['packet_count'] = 0
            flow_info['byte_count'] = 0
            flow_info['duration'] = '-'
            
            # 提取cookie
            if 'cookie=' in line:
                try:
                    cookie_part = line.split('cookie=')[1].split(',')[0].split()[0]
                    flow_info['cookie'] = cookie_part
                except:
                    flow_info['cookie'] = 'unknown'
            
            # 提取优先级
            if 'priority=' in line:
                try:
                    priority_part = line.split('priority=')[1].split(',')[0].split()[0]
                    flow_info['priority'] = int(priority_part)
                except:
                    flow_info['priority'] = 32768
            
            # 提取匹配条件和动作
            if ' actions=' in line:
                try:
                    match_actions = line.split(' actions=')
                    if len(match_actions) >= 2:
                        match_part = match_actions[0]
                        actions_part = match_actions[1]
                        
                        print(f"匹配部分: '{match_part}'")
                        print(f"动作部分: '{actions_part}'")
                        
                        # 解析匹配条件：去除统计信息和元数据，只保留真正的匹配条件
                        match_conditions = []
                        
                        # 按逗号分割所有字段
                        fields = [f.strip() for f in match_part.split(',')]
                        print(f"所有字段: {fields}")
                        
                        # 定义需要跳过的统计和元数据字段前缀
                        skip_prefixes = [
                            'cookie=', 'duration=', 'table=', 'n_packets=', 'n_bytes=', 
                            'idle_age=', 'hard_age=', 'idle_timeout=', 'hard_timeout=',
                            'send_flow_rem', 'check_overlap', 'flags=', 'importance='
                        ]
                        
                        for field in fields:
                            if not field:
                                continue
                            
                            print(f"处理字段: '{field}'")
                            
                            # 检查是否是需要跳过的统计字段
                            should_skip = any(field.startswith(prefix) for prefix in skip_prefixes)
                            
                            if should_skip:
                                print(f"  -> 跳过统计字段: {field}")
                                continue
                            
                            # priority字段需要特殊处理
                            if field.startswith('priority='):
                                # 只有当priority不等于0或者明确指定时才作为匹配条件
                                try:
                                    priority_val = int(field.split('=')[1])
                                    if priority_val != 0:
                                        match_conditions.append(field)
                                        print(f"  -> 添加优先级匹配: {field}")
                                    else:
                                        print(f"  -> 跳过默认优先级: {field}")
                                except:
                                    print(f"  -> 优先级解析失败: {field}")
                                continue
                            
                            # 其他包含等号的字段都是匹配条件
                            if '=' in field:
                                match_conditions.append(field)
                                print(f"  -> 添加匹配条件: {field}")
                            # 纯协议类型字段
                            elif field in ['tcp', 'udp', 'icmp', 'arp', 'ip', 'ipv6']:
                                match_conditions.append(field)
                                print(f"  -> 添加协议类型: {field}")
                            else:
                                print(f"  -> 忽略无关字段: {field}")
                        
                        # 生成最终的匹配条件字符串
                        if not match_conditions:
                            flow_info['match'] = 'any'
                            print("  -> 最终匹配条件: any")
                        else:
                            # 将端口号转换为端口名
                            converted_conditions = []
                            for condition in match_conditions:
                                converted_condition = convert_port_numbers_to_names(condition, port_mapping)
                                converted_conditions.append(converted_condition)
                            flow_info['match'] = ', '.join(converted_conditions)
                            print(f"  -> 最终匹配条件: {flow_info['match']}")
                        
                        # 将动作中的端口号转换为端口名
                        converted_actions = convert_port_numbers_to_names(actions_part.strip(), port_mapping)
                        flow_info['actions'] = converted_actions
                        print(f"  -> 动作: {flow_info['actions']}")
                        
                except Exception as e:
                    print(f"解析匹配条件和动作失败: {e}")
                    flow_info['match'] = 'parse_error'
                    flow_info['actions'] = '-'
            
            # 提取统计信息
            if 'n_packets=' in line:
                try:
                    packets = line.split('n_packets=')[1].split(',')[0].split()[0]
                    flow_info['packet_count'] = int(packets)
                except:
                    flow_info['packet_count'] = 0
            
            if 'n_bytes=' in line:
                try:
                    bytes_count = line.split('n_bytes=')[1].split(',')[0].split()[0]
                    flow_info['byte_count'] = int(bytes_count)
                except:
                    flow_info['byte_count'] = 0
                    
            # 提取持续时间
            if 'duration=' in line:
                try:
                    duration = line.split('duration=')[1].split(',')[0].split()[0]
                    # duration字段通常已经包含单位，不需要再添加's'
                    flow_info['duration'] = duration
                except:
                    flow_info['duration'] = '-'
            
            # 只有当至少有一些有效信息时才添加流表
            if flow_info['match'] != 'any' or flow_info['actions'] != '-' or flow_info['priority'] != 32768:
                flows.append(flow_info)
                print(f"成功解析流表: {flow_info}")  # 调试信息
            
        except Exception as e:
            print(f"解析流表行失败: {line}, 错误: {e}")
            # 即使解析失败，也尝试创建一个基本的流表项
            flows.append({
                'cookie': 'unknown',
                'priority': 32768,
                'match': line[:100] + '...' if len(line) > 100 else line,
                'actions': 'parse_error',
                'packet_count': 0,
                'byte_count': 0,
                'duration': '-'
            })
            continue
    
    print(f"总共解析出{len(flows)}条流表")  # 调试信息
    return flows



def get_ovs_interface_states(sudo_password=None):
    """获取OVS网络接口的详细状态信息"""
    try:
        # 首先尝试不使用sudo的命令（适用于容器环境或有权限的用户）
        try:
            result = subprocess.run(['ovs-vsctl', '--format=table', '--columns=name,link_state,admin_state,statistics', 'list', 'interface'], 
                                  capture_output=True, text=True, timeout=10)
            
            # 如果命令执行成功，解析结果
            if result.returncode == 0:
                return parse_ovs_interface_output(result.stdout)
            else:
                print(f"直接执行ovs-vsctl失败: {result.stderr}")
        except FileNotFoundError:
            print("ovs-vsctl命令未找到")
        except Exception as e:
            print(f"直接执行ovs-vsctl时发生错误: {e}")
        
        # 如果直接执行失败且提供了sudo密码，尝试使用sudo
        if sudo_password:
            try:
                command = f'echo "{sudo_password}" | sudo -S ovs-vsctl --format=table --columns=name,link_state,admin_state,statistics list interface'
                result = subprocess.run(command, shell=True, capture_output=True, text=True, timeout=10)
                
                if result.returncode == 0:
                    return parse_ovs_interface_output(result.stdout)
                else:
                    print(f"使用sudo执行ovs-vsctl失败: {result.stderr}")
            except Exception as e:
                print(f"使用sudo执行ovs-vsctl时发生错误: {e}")
        
        return {}
            
    except Exception as e:
        print(f"获取OVS接口状态时发生错误: {e}")
        return {}

def parse_ovs_interface_output(output):
    """解析ovs-vsctl list interface命令的输出"""
    interfaces = {}
    
    # 解析表格输出
    lines = output.strip().split('\n')
    for line in lines:
        line = line.strip()
        if not line or line.startswith('name') or line.startswith('---'):
            continue
        
        # 解析每行数据，格式类似: name link_state admin_state statistics
        parts = line.split()
        if len(parts) >= 3:
            name = parts[0].strip('"')
            link_state = parts[1] if parts[1] != '[]' else 'unknown'
            admin_state = parts[2] if parts[2] != '[]' else 'unknown'
            
            # 解析统计信息（如果有）
            statistics = {}
            if len(parts) > 3:
                stats_str = ' '.join(parts[3:])
                # 这里可以进一步解析统计信息
                statistics['raw'] = stats_str
            
            interfaces[name] = {
                'link_state': link_state,
                'admin_state': admin_state,
                'statistics': statistics
            }
    
    return interfaces

def parse_ovs_show_output(output, sudo_password=None):
    """解析ovs-ofctl show命令的输出"""
    ports = []
    lines = output.strip().split('\n')
    
    # 获取所有OVS接口的状态信息
    interface_states = get_ovs_interface_states(sudo_password)
    
    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        # 解析端口行，格式类似: 1(veth0): addr:02:42:ac:11:00:02
        import re
        port_match = re.match(r'(\d+)\(([^)]+)\):\s*addr:([a-fA-F0-9:]+)', line)
        if port_match:
            port_number = int(port_match.group(1))
            port_name = port_match.group(2)
            mac_addr = port_match.group(3)
            
            # 跳过LOCAL端口（65534端口或名称为LOCAL的端口）
            if port_name == 'LOCAL' or port_number == 65534:
                continue
            
            # 从OVS接口状态信息中获取该端口的状态
            interface_info = interface_states.get(port_name, {})
            admin_state = interface_info.get('admin_state', 'unknown')
            link_state = interface_info.get('link_state', 'unknown')
            statistics = interface_info.get('statistics', {})
                
            port_data = {
                'number': port_number,
                'name': port_name,
                'mac_address': mac_addr,
                'admin_state': admin_state,
                'link_state': link_state
            }
            
            # 如果有统计信息，也添加进去
            if statistics:
                port_data['statistics'] = statistics
                
            ports.append(port_data)
        
        # 也尝试解析简单格式的端口行
        elif re.match(r'^\d+:', line):
            try:
                parts = line.split(':')
                if len(parts) >= 2:
                    port_number = int(parts[0])
                    port_name = parts[1].strip().split()[0] if parts[1].strip() else f'port{port_number}'
                    
                    # 跳过LOCAL端口
                    if port_name == 'LOCAL' or port_number == 65534:
                        continue
                    
                    ports.append({
                        'number': port_number,
                        'name': port_name,
                        'type': 'unknown',
                        'admin_state': 'up',
                        'link_state': 'up'
                    })
            except:
                continue
    
    # 不再添加默认的LOCAL端口，如果没有实际端口就返回空列表
    return ports

def parse_port_statistics(output):
    """解析端口统计信息输出"""
    port_stats = []
    lines = output.strip().split('\n')
    
    for line in lines:
        line = line.strip()
        if not line or 'OFPST_PORT' in line:
            continue
            
        # 解析端口统计行
        # 格式通常为: port  1: rx pkts=100, bytes=6400, drop=0, errs=0, frame=0, over=0, crc=0
        #             tx pkts=80, bytes=5120, drop=0, errs=0, coll=0
        if line.startswith('port'):
            try:
                # 提取端口号
                port_num = line.split(':')[0].split()[-1]
                
                # 提取统计数据
                stats_part = line.split(':', 1)[1] if ':' in line else line
                
                # 解析RX和TX统计
                rx_stats = {}
                tx_stats = {}
                
                # 简单的统计解析
                import re
                rx_match = re.search(r'rx pkts=(\d+).*?bytes=(\d+)', stats_part)
                tx_match = re.search(r'tx pkts=(\d+).*?bytes=(\d+)', stats_part)
                
                if rx_match:
                    rx_stats = {
                        'packets': int(rx_match.group(1)),
                        'bytes': int(rx_match.group(2))
                    }
                
                if tx_match:
                    tx_stats = {
                        'packets': int(tx_match.group(1)), 
                        'bytes': int(tx_match.group(2))
                    }
                
                port_stats.append({
                    'port_number': port_num,
                    'rx_stats': rx_stats,
                    'tx_stats': tx_stats,
                    'raw_line': line
                })
                
            except Exception as e:
                print(f"解析端口统计行失败: {line}, 错误: {e}")
                # 添加原始行作为备用
                port_stats.append({
                    'port_number': 'unknown',
                    'raw_line': line,
                    'parse_error': str(e)
                })
    
    return port_stats

def extract_ports_from_flows(flows):
    """从流表信息中提取端口信息"""
    ports = set()
    port_info = {}
    
    for flow in flows:
        actions = flow.get('actions', '')
        match = flow.get('match', '')
        
        # 从动作中提取输出端口
        import re
        output_matches = re.findall(r'output:(\w+)', actions)
        for port in output_matches:
            if port.isdigit():
                port_num = int(port)
                ports.add(port_num)
                if port_num not in port_info:
                    port_info[port_num] = {
                        'number': port_num,
                        'name': f'port{port_num}',
                        'type': 'unknown'
                    }
        
        # 从匹配条件中提取输入端口
        in_port_matches = re.findall(r'in_port=(\w+)', match)
        for port in in_port_matches:
            if port.isdigit():
                port_num = int(port)
                ports.add(port_num)
                if port_num not in port_info:
                    port_info[port_num] = {
                        'number': port_num,
                        'name': f'port{port_num}',
                        'type': 'unknown'
                    }
    
    return list(port_info.values())

def get_detailed_port_info(container_ip, bridge, sudo_password, timeout):
    """尝试获取详细的端口信息"""
    try:
        # 构建一个通用的端口查询API调用
        # 这里使用一个假设的API，实际可能需要调整
        container_url = f"http://{container_ip}:5000/api/bridge-details/{bridge}"
        request_data = {}
        if sudo_password:
            request_data['sudo_password'] = sudo_password
        
        response = requests.post(container_url, json=request_data, timeout=timeout)
        
        if response.status_code == 200:
            data = response.json()
            return data.get('ports', [])
        else:
            return []
    except:
        return []

def merge_port_info(basic_ports, detailed_ports):
    """合并基础端口信息和详细端口信息"""
    merged = {}
    
    # 添加基础端口信息
    for port in basic_ports:
        port_num = port.get('number')
        if port_num is not None:
            merged[port_num] = port.copy()
    
    # 合并详细信息
    for port in detailed_ports:
        port_num = port.get('number') or port.get('port_no')
        if port_num is not None:
            if port_num in merged:
                merged[port_num].update(port)
            else:
                merged[port_num] = port
    
    # 如果没有端口信息，创建一些默认端口
    if not merged:
        merged[1] = {
            'number': 1,
            'name': 'LOCAL',
            'type': 'internal',
            'admin_state': 'up'
        }
    
    return list(merged.values())

def build_action_string(action_data):
    """构建单个动作字符串，采用 action(protocol,key=value) 格式"""
    action_type = action_data.get('type')
    
    # 特殊处理output动作
    if action_type == 'output':
        parameter = action_data.get('parameter', '')
        return f'output:{parameter}'
    
    # 特殊处理clone动作
    if action_type == 'clone':
        nested_actions = action_data.get('nested_actions', [])
        if nested_actions:
            # 递归处理嵌套动作
            nested_action_strs = []
            for nested_action in nested_actions:
                nested_str = build_action_string(nested_action)
                nested_action_strs.append(nested_str)
            nested_actions_str = ','.join(nested_action_strs)
            return f'clone({nested_actions_str})'
        else:
            return 'clone()'
    
    protocol = action_data.get('protocol')
    params = action_data.get('params', {})
    
    if params:
        field_params = ",".join([f"{field}={value}" for field, value in params.items()])
        return f'{action_type}({protocol},{field_params})'
    else:
        return f'{action_type}({protocol})'

def calculate_port_output_from_flows(flows, port_mapping):
    """从流表信息中计算每个端口的输出数据包数"""
    import re
    
    # 初始化端口输出统计
    port_output_stats = {}
    
    # 创建端口名到端口号的反向映射
    name_to_number = {name: number for number, name in port_mapping.items()}
    
    for flow in flows:
        actions = flow.get('actions', '')
        packet_count = flow.get('packet_count', 0)
        byte_count = flow.get('byte_count', 0)
        
        if not actions or packet_count == 0:
            continue
            
        # 查找所有output动作
        # 匹配 output:端口名 或 output:端口号
        output_matches = re.findall(r'output:(\w+)', actions)
        
        for output_target in output_matches:
            port_number = None
            port_name = None
            
            # 判断是端口号还是端口名
            if output_target.isdigit():
                port_number = output_target
                port_name = port_mapping.get(output_target, f'port{output_target}')
            else:
                port_name = output_target
                port_number = name_to_number.get(output_target, output_target)
            
            # 初始化端口统计
            if port_number not in port_output_stats:
                port_output_stats[port_number] = {
                    'port_number': port_number,
                    'port_name': port_name,
                    'output_packets': 0,
                    'output_bytes': 0,
                    'flow_count': 0  # 输出到该端口的流表数量
                }
            
            # 累加数据包和字节数
            port_output_stats[port_number]['output_packets'] += packet_count
            port_output_stats[port_number]['output_bytes'] += byte_count
            port_output_stats[port_number]['flow_count'] += 1
    
    # 转换为列表格式
    result = []
    for port_stat in port_output_stats.values():
        result.append({
            'port_number': port_stat['port_number'],
            'port_name': port_stat['port_name'],
            'output_packets': port_stat['output_packets'],
            'output_bytes': port_stat['output_bytes'],
            'flow_count': port_stat['flow_count'],
            'source': 'flow_table'  # 标识数据来源是流表
        })
    
    return result

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)