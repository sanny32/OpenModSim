check_openssl_version() {
    case "$1" in
        5) pkg-config --exists 'openssl >= 1.1.1' 'openssl < 3' ;;
        6) pkg-config --exists 'openssl >= 3' 'openssl < 4' ;;
        *) return 1 ;;
    esac
}
