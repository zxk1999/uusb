import re

# log_file = "mmylog2.txt"
log_file = "myloog_3_18_15_46.txt"
BLOCK_TYPES = {
    ##ep_kick->(ep fetch_trb->(xfer_start->xfer_comp)->queue event)
    # "Device Connection and Initialization": {
    #     "start": ["usb_xhci_port_link"],
    #     "end": ["usb_xhci_slot_enable"]
    # },
    # "Endpoint Configuration": {
    #     "start": ["usb_xhci_ep_enable"],
    #     "end": ["usb_xhci_ep_disable"]
    # },
    "Data Transfer": {

        "start": ["MMIO Read: addr=0xfebd0044"],
        "end": ["MMIO Write: addr=0xfebd2004, len=4, val=0x7"]

    },

    "Control Transfer": {
        "start": ["MMIO Read: addr=0xfebd0044"],
        "end": ["MMIO Write: addr=0xfebd2004, len=4, val=0x1"]
    },


    # "Event Handling": {
    #     "start": ["usb_xhci_queue_event"],
    #     "end": ["usb_xhci_queue_event"]
    # },
    # "Error Handling": {
    #     "start": ["usb_xhci_ep_state"],
    #     "end": ["usb_xhci_ep_reset", "usb_xhci_ep_disable"]
    # },
    # "state change": {
    #     "start": ["usb_xhci_ep_state"],
    #    "end": ["usb_xhci_ep_reset", "usb_xhci_ep_disable"]
    # },
    # "Device Disconnection and Cleanup": {
    #     "start": ["usb_xhci_port_link", "usb_xhci_port_notify"],
    #     "end": ["usb_xhci_slot_disable"]
    # }
}


colors = {
    'Device Connection and Initialization': 'skyblue',
    'control_transfer': 'lightgreen',
    'isoc_transfer': 'lightcoral',
    'Data Transfer': "lightgreen"
}

x_size = {
    'Device Connection and Initialization': 2,
    'control_transfer': 4,
    'isoc_transfer': 6,
    'Data Transfer': 8
}

control_trans=["TR_SETUP","TR_DATA","TR_STATUS"]
isoc_trans=["TR_ISOCH"]
with open(log_file, "r") as f:
    log_lines = f.readlines()

basic_blocks = []
start=1
current_block = {"type": "", "lines": []}
for line in log_lines:
    matched = False
    line = line.strip()
    if not line:
        continue
    
    if start:
        for block_type, conditions in BLOCK_TYPES.items():
            if (any(keyword in line for keyword in conditions["start"])):
                start=0

    if start:
        continue
    # print(line)
    for block_type, conditions in BLOCK_TYPES.items():
        if any(keyword in line for keyword in conditions["end"]):
            if current_block is not None:
                current_block["lines"].append(line)
                current_block["type"] = block_type
                basic_blocks.append(current_block)
            current_block = {"type": block_type, "lines": []}
            matched = True
            break  

    if not matched:
        if current_block is not None:
            # if any(keyword in line for keyword in control_trans):
            #     current_block["type"]="control_transfer"
            # if any(keyword in line for keyword in isoc_trans):
            #     current_block["type"]="isoc_transfer"
            current_block["lines"].append(line)
        else:
            print('error') 
    # if len(basic_blocks)==1:
    #     break

if current_block:
    basic_blocks.append(current_block)

with open('output_3_18_myloog.txt', 'w') as f:
    for idx, block in enumerate(basic_blocks):
        print(f"Block {idx + 1} - Type: {block['type']}",file=f)
        for line in block["lines"]:
            print(f"  {line}",file=f)
        print("\n",file=f)

    block_relations = []
    for i in range(len(basic_blocks) - 1):
        current_block = basic_blocks[i]
        next_block = basic_blocks[i + 1]
        block_relations.append((current_block["type"], next_block["type"]))
        # block_relations.append((current_block["lines"], next_block["lines"]))

    print("Block Relations:",file=f)
    for relation in block_relations:
        print(f"{relation[0]} -> {relation[1]}",file=f)



# import matplotlib.pyplot as plt
# import networkx as nx

# G = nx.DiGraph()
# node_sizes=[]
# # 添加节点和边
# for idx,block in enumerate(basic_blocks):
#     G.add_node('\n'.join(str(item) for item in block["lines"])+str(idx))
#     node_sizes.append(len('\n'.join(str(item) for item in block["lines"])+str(idx))*19)
#     #G.add_node(block["lines"])

# edges=[]
# for idx,relation in enumerate(block_relations):
#     edges.append(('\n'.join(str(item) for item in relation[0])+str(idx),'\n'.join(str(item) for item in relation[1])+str(idx+1)))
#     # edges.append((relation[0],relation[1]))
# for edge in edges:
#     G.add_edge(*edge)


# # 获取所有块名并按字母顺序排序

# # 计算布局
# pos = {}
# node_colors = []
# current_x = 0
# print(max(node_sizes))
# node_spacing = -max(node_sizes)  # 每个节点间的垂直间距
# max_y = len(basic_blocks) * node_spacing  # 根据所有块的数量确定最大y值

# fig, ax = plt.subplots(figsize=(12, 8))

# # 为每个类型分配列位置，并为每个块分配唯一的纵坐标
# for idx, block in enumerate(basic_blocks):
#     # 设置每列的背景颜色
#     ax.add_patch(plt.Rectangle((current_x - 0.5, max_y), 5, abs(max_y)+10,
#                                color=colors[block["type"]], alpha=0.3))
    
#     # 在每一类block的顶部添加一个虚拟节点作为标题
#     # title_pos = (current_x + 0.5, max_y + 0.5)
#     # plt.text(title_pos[0], title_pos[1], block["type"], fontsize=12, ha='center', va='bottom', color='black')  # 添加标题
    
#     # 找到当前块在整个列表中的索引，以确定其纵坐标
#     block_index = basic_blocks.index(block)
#     pos['\n'.join(str(item) for item in block["lines"])+str(idx)] = (x_size[block["type"]], block_index * node_spacing)
#     node_colors.append(colors[block_type])
    
#     #current_x += 2  # 改变这个值可以调整列间的距离

# # 绘制图形
# nx.draw(G, pos, with_labels=True, node_size=node_sizes, node_color=node_colors, font_size=5, font_weight="bold", arrowsize=20, ax=ax)
# plt.title("ISOC Block")
# plt.axis('off')  # 关闭坐标轴

# plt.show()