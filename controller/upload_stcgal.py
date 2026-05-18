Import("env")

protocol = env.GetProjectOption("custom_stcgal_protocol", "stc8g")
baud = env.GetProjectOption("custom_stcgal_baud", "19200")
handshake_baud = env.GetProjectOption("custom_stcgal_handshake_baud", "").strip()
trim = env.GetProjectOption("custom_stcgal_trim", "").strip()
attempts = env.GetProjectOption("custom_stcgal_attempts", "2").strip()

env.Replace(
    UPLOADCMD=(
        '"$PYTHONEXE" ../tools/upload_stcgal_runner.py '
        '--python "$PYTHONEXE" '
        '--uploader "$UPLOADER" '
        '--protocol %s '
        '--port "$UPLOAD_PORT" '
        '--source "$SOURCE" '
        '--baud %s '
        '--handshake-baud "%s" '
        '--trim "%s" '
        '--attempts %s'
    ) % (protocol, baud, handshake_baud, trim, attempts)
)
