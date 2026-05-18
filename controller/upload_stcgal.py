Import("env")

protocol = env.GetProjectOption("custom_stcgal_protocol", "stc8g")
baud = env.GetProjectOption("custom_stcgal_baud", "38400")
trim = env.GetProjectOption("custom_stcgal_trim", "").strip()

trim_flags = ("-t %s " % trim) if trim else ""

env.Replace(
    UPLOADCMD=(
        '"$PYTHONEXE" "$UPLOADER" '
        '-P %s '
        '-p "$UPLOAD_PORT" '
        '%s'
        '-a '
        '-b %s '
        '$SOURCE'
    ) % (protocol, trim_flags, baud)
)
