# ============================================================================
#  build_hex.py —— PlatformIO 后处理脚本：链接成功后用 objcopy 生成 .hex / .bin
# ============================================================================
Import("env")

# 由 ELF 生成 Intel HEX（用于串口下载工具 / FlyMcu / Flash Loader 等）
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(
        '$OBJCOPY -O ihex "$BUILD_DIR/${PROGNAME}.elf" "$BUILD_DIR/${PROGNAME}.hex"',
        "Generating HEX  $BUILD_DIR/${PROGNAME}.hex",
    ),
)

# 由 ELF 生成纯二进制 BIN
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(
        '$OBJCOPY -O binary "$BUILD_DIR/${PROGNAME}.elf" "$BUILD_DIR/${PROGNAME}.bin"',
        "Generating BIN  $BUILD_DIR/${PROGNAME}.bin",
    ),
)
