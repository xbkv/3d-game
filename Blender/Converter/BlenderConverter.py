with open("C:/Users/ryuai/OneDrive/ドキュメント/blender/obj/tunnel.obj", "r", encoding="utf-8") as f:
    data = f.read()
v = []
v_f = []
vt = []
vn = []
f = []
num = 0
for i in data.split("\n"):
    if i.startswith("v "):
        for j in i.split():
            if not j.startswith("v"):
                v_f.append(str(format(round(float(j), 2), '.2f')) + "f")
                num += 1
                if num == 3:
                    v.append(list(v_f))
                    num = 0
                    v_f.clear()
    elif i.startswith("vt "):
        for j in i.split():
            if not j.startswith("vt"):
                v_f.append(str(format(round(float(j), 2), '.2f')) + "f")
                num += 1
                if num == 2:
                    vt.append(list(v_f))
                    num = 0
                    v_f.clear()
    elif i.startswith("vn "):
        for j in i.split():
            if not j.startswith("vn"):
                v_f.append(str(format(round(float(j), 2), '.2f')) + "f")
                num += 1
                if num == 3:
                    vn.append(list(v_f))
                    num = 0
                    v_f.clear()
    elif i.startswith("f "):
        for j in i.split():
            if not j.startswith("f"):
                f.append(j)

for k in f:
    v_f.append(k.split("/"))


import re

# パターンマッチングで正規表現を用いて3つの数字を見つける
pattern = re.compile(r'\d+/\d+/\d+', re.DOTALL)
matches = pattern.findall(data)
# 各マッチングに対してv, vt, vnを抽出し、指定のフォーマットで出力する
with open("result.txt", "w") as f:
    f.write("")
with open("result.txt", "a") as f:
    f.write("// Triangle\n")
    for match in matches:
        indices = re.findall(r'\d+/\d+/\d+', match)
        for index in indices:
            v_index, vt_index, vn_index = index.split("/")
            f.write(
                f"\t{{ {{"
                f"{', '.join(v[int(v_index) - 1])}"
                f"}}, {{"
                f"{', '.join(vt[int(vt_index) - 1])}"
                f"}}, {{"
                f"{', '.join(vn[int(vn_index) - 1])}"
                f"}} }},\n"
            )