import network_monitor
import gradio as gr

def fetch_data():
    try:
        data = network_monitor.get_data()
        
        basic_info = (f"IPv4: {data['ipv4']}\n"
                      f"Subnet: {data['subnet']}\n"
                      f"Gateway: {data['gateway']}\n"
                      f"MAC: {data['mac']}\n"
                      f"DNS: {data['dns']}")
                      
        traffic = (f"Download: {data['download']}\n"
                   f"Upload: {data['upload']}\n"
                   f"Ping: {data['ping']}")
                   
        packets = (f"In Packets: {data['inPackets']}\n"
                   f"Out Packets: {data['outPackets']}\n"
                   f"In Errors: {data['inErrors']}\n"
                   f"Out Errors: {data['outErrors']}\n"
                   f"Loss Rate: {data['lossRate']:.2f}%")
                   
        conns = (f"TCP Connections: {data['tcpCount']}\n"
                 f"UDP Connections: {data['udpCount']}")
                 
        ifaces = "\n".join([f"[{i+1}] {iface['name']} ({iface['status']}, {iface['speed_mbps']:.2f} Mbps)" for i, iface in enumerate(data['interfaces'])])
        
        procs = "\n".join([f"PID: {p['pid']}, Process: {p['name']}" for p in data['processes'][:15]])
        
        return basic_info, traffic, packets, conns, ifaces, procs
    except Exception as e:
        return str(e), "", "", "", "", ""

custom_css = """
footer {display: none !important;}
"""

with gr.Blocks(theme=gr.themes.Soft(), css=custom_css) as demo:
    gr.Markdown("# 🌐 실시간 네트워크 모니터링 전체 대시보드")
    
    with gr.Row():
        box_basic = gr.Textbox(label="■ 네트워크 주소 ■", lines=6, show_copy_button=False)
        box_traffic = gr.Textbox(label="■ 네트워크 성능 ■", lines=4, show_copy_button=False)
    
    with gr.Row():
        box_packets = gr.Textbox(label="■ 패킷 ■", lines=6, show_copy_button=False)
        box_conns = gr.Textbox(label="■ 연결 상태 ■", lines=3, show_copy_button=False)
        
    with gr.Row():
        box_ifaces = gr.Textbox(label="■ 네트워크 인터페이스 ■", lines=6, show_copy_button=False)
        box_procs = gr.Textbox(label="■ 프로세스별 네트워크 사용량 (최대 15개) ■", lines=15, show_copy_button=False)
        
    timer = gr.Timer(8)
    timer.tick(fetch_data, inputs=None, outputs=[box_basic, box_traffic, box_packets, box_conns, box_ifaces, box_procs])

if __name__ == "__main__":
    demo.launch(server_port=7860, show_api=False)