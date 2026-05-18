Import("env")

protocol = env.GetProjectOption("custom_stcgal_protocol", "stc8g")
baud = env.GetProjectOption("custom_stcgal_baud", "19200")
handshake_baud = env.GetProjectOption("custom_stcgal_handshake_baud", "").strip()
trim = env.GetProjectOption("custom_stcgal_trim", "").strip()

handshake_flags = ("-l %s " % handshake_baud) if handshake_baud else ""
trim_flags = ("-t %s " % trim) if trim else ""

env.Replace(
    UPLOADCMD=(
        '"$PYTHONEXE" "$UPLOADER" '
        '-P %s '
        '-p "$UPLOAD_PORT" '
        '%s'
        '%s'
        '-a '
        '-b %s '
        '$SOURCE'
    ) % (protocol, handshake_flags, trim_flags, baud)
)
