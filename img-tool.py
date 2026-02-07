from PIL import Image

def load_img(img_path:str):
    # Turn image into pixel array, height and width
    img = Image.open(img_path).convert('L')
    w, h = img.size
    print("height, width", (h, w))
    pixels = list(img.get_flattened_data())

    return pixels, h, w

def rle_encode(pixels:list[int], width: int, out_path:str):
    # Will generate the .cwp file that you can load onto the calculator
    streak = 1
    px = pixels[0]
    res = [((px != 0) << 7) | (width > 255), width%256]

    for p in pixels[1:]:
        if p == px:
            streak += 1
            if streak == 256:
                res.append(255)
                res.append(0)
                streak = 1
        else:
            res.append(streak)
            streak = 1
            px = p

    if streak > 0:
        res.append(streak)

    # print(res, len(res))
    print("File size in bytes:", len(res))

    with open(out_path, "w") as file:
        file.write(''.join(chr(r) for r in res))

pixels, H, W = load_img('resources/glider_gun.png') # Use your own image path
rle_encode(pixels, W, "src/pattern0.cwp") # You can change the path


