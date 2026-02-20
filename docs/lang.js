const translations = {
    en: {
        'nav.features': 'Features',
        'nav.screenshots': 'Screenshots',
        'nav.download': 'Download',
        'hero.title': 'Free and powerful Modbus Simulator',
        'hero.subtitle': 'Professional Modbus Slave (Server) simulator supporting both Modbus-TCP and Modbus-RTU protocols',
        'hero.btn.download': 'Download',
        'hero.btn.github': 'View on GitHub',
        'features.title': 'Features',
        'feature1.title': 'Modbus Functions',
        'feature1.desc': 'Support for all standard Modbus functions: Read Coils, Read Discrete Inputs, Write Single/Multiple Coils, Read/Write Holding Registers',
        'feature2.title': 'Data Simulation',
        'feature2.desc': 'Simulate coil/flag values with Random and Toggle modes. Simulate register values with Random, Increment, and Decrement modes with configurable limits and step',
        'feature3.title': 'Message Logging',
        'feature3.desc': 'Comprehensive logging of all Modbus communication traffic with built-in Message Parser for analyzing and decoding protocol frames',
        'feature4.title': 'Error Simulation',
        'feature4.desc': 'Simulate Modbus error responses and exception codes to thoroughly test master (client) device error handling',
        'feature5.title': 'Scripting',
        'feature5.desc': 'ECMAScript/JavaScript scripting support for advanced simulation scenarios. Run scripts Once or Periodically with full access to register and coil values',
        'feature6.title': 'Cross-Platform',
        'feature6.desc': 'Available for Windows 7+ and major Linux distributions (Debian, Ubuntu, Fedora, Rocky, OpenSUSE, Alt Linux, Astra Linux, RedOS) with native performance',
        'screenshots.title': 'Screenshots',
        'download.title': 'Download OpenModSim',
        'download.subtitle': 'Get the latest version for your platform',
        'card.windows.title': 'Windows Installer',
        'card.sysreq.title': 'System Requirements',
        'card.deb.title': 'Linux DEB Packages',
        'card.rpm.title': 'Linux RPM Packages',
        'card.flatpak.title': 'Flatpak Packages',
        'btn.download.win': 'Download Windows Installer',
        'btn.download.deb': 'Download DEB Package',
        'btn.download.rpm': 'Download RPM Package',
        'btn.download.pubkey': 'Download RPM Public Key',
        'btn.download.flatpak': 'Download Flatpak bundle',
        'subtab.install': 'Install',
        'subtab.remove': 'Remove',
        'sysreq.win7': 'Windows 7 or newer for Qt5 packages',
        'sysreq.win10': 'Windows 10 or newer for Qt6 packages',
        'sysreq.arch': '32-bit (x86) or 64-bit (x64) processor',
        'install.qt6': 'Qt6 package',
        'install.qt5': 'Qt5 package',
        'suse.import': 'Import public key and install',
        'altlinux.root': 'Run as root user',
        'flatpak.tip': 'For serial port connections, add the user to the <code>dialout</code> group, then log in again or reboot',
        'build.title': 'Build from Source',
        'build.desc': 'OpenModSim is open source and can be built from source code. Visit the <a href="https://github.com/sanny32/OpenModSim#building">GitHub repository</a> for build instructions.',
        'footer.brand.desc': 'Free and open-source Modbus Slave simulator',
        'footer.links': 'Links',
        'footer.github': 'GitHub Repository',
        'footer.releases': 'Releases',
        'footer.issues': 'Report Issues',
        'footer.license': 'License',
        'footer.license.text': 'Licensed under MIT License',
        'footer.license.free': 'Free to use, modify and distribute'
    },
    ru: {
        'nav.features': 'Возможности',
        'nav.screenshots': 'Скриншоты',
        'nav.download': 'Скачать',
        'hero.title': 'Бесплатный и мощный Modbus симулятор',
        'hero.subtitle': 'Профессиональный симулятор Modbus Slave (Server) с поддержкой протоколов Modbus-TCP и Modbus-RTU',
        'hero.btn.download': 'Скачать',
        'hero.btn.github': 'Открыть на GitHub',
        'features.title': 'Возможности',
        'feature1.title': 'Функции Modbus',
        'feature1.desc': 'Поддержка всех стандартных функций Modbus: чтение катушек, чтение дискретных входов, запись одиночных/множественных катушек, чтение/запись регистров хранения',
        'feature2.title': 'Симуляция данных',
        'feature2.desc': 'Симуляция значений катушек/флагов в режимах «Случайный» и «Переключение». Симуляция значений регистров в режимах «Случайный», «Инкремент» и «Декремент» с настраиваемыми лимитами и шагом',
        'feature3.title': 'Журналирование сообщений',
        'feature3.desc': 'Полное журналирование всего трафика Modbus с встроенным анализатором сообщений для разбора и декодирования протокольных кадров',
        'feature4.title': 'Симуляция ошибок',
        'feature4.desc': 'Симуляция ответов с ошибками Modbus и кодами исключений для полноценного тестирования обработки ошибок на стороне ведущего (клиентского) устройства',
        'feature5.title': 'Скриптинг',
        'feature5.desc': 'Поддержка скриптов ECMAScript/JavaScript для сложных сценариев симуляции. Запуск скриптов однократно или периодически с полным доступом к значениям регистров и катушек',
        'feature6.title': 'Кроссплатформенность',
        'feature6.desc': 'Доступен для Windows 7+ и основных дистрибутивов Linux (Debian, Ubuntu, Fedora, Rocky, OpenSUSE, Alt Linux, Astra Linux, RedOS) с нативной производительностью',
        'screenshots.title': 'Скриншоты',
        'download.title': 'Скачать OpenModSim',
        'download.subtitle': 'Получите последнюю версию для вашей платформы',
        'card.windows.title': 'Установщик Windows',
        'card.sysreq.title': 'Системные требования',
        'card.deb.title': 'Пакеты Linux DEB',
        'card.rpm.title': 'Пакеты Linux RPM',
        'card.flatpak.title': 'Пакеты Flatpak',
        'btn.download.win': 'Скачать установщик Windows',
        'btn.download.deb': 'Скачать пакет DEB',
        'btn.download.rpm': 'Скачать пакет RPM',
        'btn.download.pubkey': 'Скачать публичный ключ RPM',
        'btn.download.flatpak': 'Скачать пакет Flatpak',
        'subtab.install': 'Установка',
        'subtab.remove': 'Удаление',
        'sysreq.win7': 'Windows 7 или новее для пакетов Qt5',
        'sysreq.win10': 'Windows 10 или новее для пакетов Qt6',
        'sysreq.arch': '32-битный (x86) или 64-битный (x64) процессор',
        'install.qt6': 'Пакет Qt6',
        'install.qt5': 'Пакет Qt5',
        'suse.import': 'Импортировать публичный ключ и установить',
        'altlinux.root': 'Запустить от имени пользователя root',
        'flatpak.tip': 'Для подключения через последовательный порт добавьте пользователя в группу <code>dialout</code>, затем выполните повторный вход или перезагрузку',
        'build.title': 'Сборка из исходного кода',
        'build.desc': 'OpenModSim является программным обеспечением с открытым исходным кодом и может быть собран из исходников. Посетите <a href="https://github.com/sanny32/OpenModSim#building">репозиторий GitHub</a> для получения инструкций по сборке.',
        'footer.brand.desc': 'Бесплатный симулятор Modbus Slave с открытым исходным кодом',
        'footer.links': 'Ссылки',
        'footer.github': 'Репозиторий GitHub',
        'footer.releases': 'Релизы',
        'footer.issues': 'Сообщить о проблеме',
        'footer.license': 'Лицензия',
        'footer.license.text': 'Лицензировано под MIT License',
        'footer.license.free': 'Бесплатно для использования, изменения и распространения'
    },
    zh_CN: {
        'nav.features': '功能',
        'nav.screenshots': '截图',
        'nav.download': '下载',
        'hero.title': '免费且强大的 Modbus 模拟器',
        'hero.subtitle': '专业的 Modbus 从站（服务器）模拟器，支持 Modbus-TCP 和 Modbus-RTU 协议',
        'hero.btn.download': '下载',
        'hero.btn.github': '在 GitHub 查看',
        'features.title': '功能特性',
        'feature1.title': 'Modbus 功能',
        'feature1.desc': '支持所有标准 Modbus 功能：读取线圈、读取离散输入、写入单个/多个线圈、读取/写入保持寄存器',
        'feature2.title': '数据模拟',
        'feature2.desc': '使用随机和切换模式模拟线圈/标志值。使用随机、递增和递减模式模拟寄存器值，可配置限制和步长',
        'feature3.title': '消息记录',
        'feature3.desc': '全面记录所有 Modbus 通信流量，内置消息解析器用于分析和解码协议帧',
        'feature4.title': '错误模拟',
        'feature4.desc': '模拟 Modbus 错误响应和异常代码，全面测试主站（客户端）设备的错误处理能力',
        'feature5.title': '脚本功能',
        'feature5.desc': '支持 ECMAScript/JavaScript 脚本，用于高级模拟场景。可单次或周期性运行脚本，完全访问寄存器和线圈值',
        'feature6.title': '跨平台',
        'feature6.desc': '适用于 Windows 7+ 和主要 Linux 发行版（Debian、Ubuntu、Fedora、Rocky、OpenSUSE、Alt Linux、Astra Linux、RedOS），具有原生性能',
        'screenshots.title': '截图',
        'download.title': '下载 OpenModSim',
        'download.subtitle': '获取适合您平台的最新版本',
        'card.windows.title': 'Windows 安装程序',
        'card.sysreq.title': '系统要求',
        'card.deb.title': 'Linux DEB 软件包',
        'card.rpm.title': 'Linux RPM 软件包',
        'card.flatpak.title': 'Flatpak 软件包',
        'btn.download.win': '下载 Windows 安装程序',
        'btn.download.deb': '下载 DEB 软件包',
        'btn.download.rpm': '下载 RPM 软件包',
        'btn.download.pubkey': '下载 RPM 公钥',
        'btn.download.flatpak': '下载 Flatpak 包',
        'subtab.install': '安装',
        'subtab.remove': '卸载',
        'sysreq.win7': 'Qt5 软件包需要 Windows 7 或更高版本',
        'sysreq.win10': 'Qt6 软件包需要 Windows 10 或更高版本',
        'sysreq.arch': '32 位（x86）或 64 位（x64）处理器',
        'install.qt6': 'Qt6 软件包',
        'install.qt5': 'Qt5 软件包',
        'suse.import': '导入公钥并安装',
        'altlinux.root': '以 root 用户运行',
        'flatpak.tip': '对于串口连接，将用户添加到 <code>dialout</code> 组，然后重新登录或重启',
        'build.title': '从源码构建',
        'build.desc': 'OpenModSim 是开源软件，可以从源代码构建。请访问 <a href="https://github.com/sanny32/OpenModSim#building">GitHub 仓库</a> 获取构建说明。',
        'footer.brand.desc': '免费开源的 Modbus 从站模拟器',
        'footer.links': '链接',
        'footer.github': 'GitHub 仓库',
        'footer.releases': '发布版本',
        'footer.issues': '提交问题',
        'footer.license': '许可证',
        'footer.license.text': '根据 MIT 许可证授权',
        'footer.license.free': '可免费使用、修改和分发'
    },
    zh_TW: {
        'nav.features': '功能',
        'nav.screenshots': '截圖',
        'nav.download': '下載',
        'hero.title': '免費且強大的 Modbus 模擬器',
        'hero.subtitle': '專業的 Modbus 從站（伺服器）模擬器，支援 Modbus-TCP 和 Modbus-RTU 協定',
        'hero.btn.download': '下載',
        'hero.btn.github': '在 GitHub 查看',
        'features.title': '功能特性',
        'feature1.title': 'Modbus 功能',
        'feature1.desc': '支援所有標準 Modbus 功能：讀取線圈、讀取離散輸入、寫入單個/多個線圈、讀取/寫入保持暫存器',
        'feature2.title': '資料模擬',
        'feature2.desc': '使用隨機和切換模式模擬線圈/標誌值。使用隨機、遞增和遞減模式模擬暫存器值，可配置限制和步長',
        'feature3.title': '訊息記錄',
        'feature3.desc': '全面記錄所有 Modbus 通信流量，內建訊息解析器用於分析和解碼協定幀',
        'feature4.title': '錯誤模擬',
        'feature4.desc': '模擬 Modbus 錯誤回應和異常代碼，全面測試主站（客戶端）設備的錯誤處理能力',
        'feature5.title': '腳本功能',
        'feature5.desc': '支援 ECMAScript/JavaScript 腳本，用於進階模擬場景。可單次或週期性執行腳本，完全存取暫存器和線圈值',
        'feature6.title': '跨平台',
        'feature6.desc': '適用於 Windows 7+ 和主要 Linux 發行版（Debian、Ubuntu、Fedora、Rocky、OpenSUSE、Alt Linux、Astra Linux、RedOS），具有原生性能',
        'screenshots.title': '截圖',
        'download.title': '下載 OpenModSim',
        'download.subtitle': '獲取適合您平台的最新版本',
        'card.windows.title': 'Windows 安裝程式',
        'card.sysreq.title': '系統需求',
        'card.deb.title': 'Linux DEB 套件',
        'card.rpm.title': 'Linux RPM 套件',
        'card.flatpak.title': 'Flatpak 套件',
        'btn.download.win': '下載 Windows 安裝程式',
        'btn.download.deb': '下載 DEB 套件',
        'btn.download.rpm': '下載 RPM 套件',
        'btn.download.pubkey': '下載 RPM 公鑰',
        'btn.download.flatpak': '下載 Flatpak 包',
        'subtab.install': '安裝',
        'subtab.remove': '解除安裝',
        'sysreq.win7': 'Qt5 套件需要 Windows 7 或更新版本',
        'sysreq.win10': 'Qt6 套件需要 Windows 10 或更新版本',
        'sysreq.arch': '32 位（x86）或 64 位（x64）處理器',
        'install.qt6': 'Qt6 套件',
        'install.qt5': 'Qt5 套件',
        'suse.import': '匯入公鑰並安裝',
        'altlinux.root': '以 root 使用者執行',
        'flatpak.tip': '對於串口連接，將使用者新增至 <code>dialout</code> 群組，然後重新登入或重新啟動',
        'build.title': '從原始碼構建',
        'build.desc': 'OpenModSim 是開源軟體，可以從原始碼構建。請訪問 <a href="https://github.com/sanny32/OpenModSim#building">GitHub 倉庫</a> 獲取構建說明。',
        'footer.brand.desc': '免費開源的 Modbus 從站模擬器',
        'footer.links': '連結',
        'footer.github': 'GitHub 倉庫',
        'footer.releases': '發布版本',
        'footer.issues': '提交問題',
        'footer.license': '授權',
        'footer.license.text': '根據 MIT 授權條款授權',
        'footer.license.free': '可免費使用、修改和分發'
    }
};

const langMeta = {
    en:    { flag: 'https://flagcdn.com/w20/us.png', name: 'English' },
    ru:    { flag: 'https://flagcdn.com/w20/ru.png', name: 'Русский' },
    zh_CN: { flag: 'https://flagcdn.com/w20/cn.png', name: '简体中文' },
    zh_TW: { flag: 'https://flagcdn.com/w20/tw.png', name: '繁體中文' }
};

function detectBrowserLang() {
    const lang = navigator.language || navigator.userLanguage || 'en';
    if (lang.startsWith('zh-TW') || lang.startsWith('zh-HK') || lang.startsWith('zh-MO')) return 'zh_TW';
    if (lang.startsWith('zh')) return 'zh_CN';
    if (lang.startsWith('ru')) return 'ru';
    return 'en';
}

function applyTranslations(lang) {
    const t = translations[lang] || translations['en'];

    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.getAttribute('data-i18n');
        if (t[key] !== undefined) el.textContent = t[key];
    });

    document.querySelectorAll('[data-i18n-html]').forEach(el => {
        const key = el.getAttribute('data-i18n-html');
        if (t[key] !== undefined) el.innerHTML = t[key];
    });

    // Update dropdown button label
    const meta = langMeta[lang] || langMeta['en'];
    const flagEl = document.querySelector('.lang-current-flag');
    const nameEl = document.querySelector('.lang-current-name');
    if (flagEl) flagEl.src = meta.flag;
    if (nameEl) nameEl.textContent = meta.name;

    // Mark active option
    document.querySelectorAll('.lang-option').forEach(opt => {
        opt.classList.toggle('active', opt.dataset.lang === lang);
    });

    // Switch screenshots to locale-specific versions (fallback to 'en')
    document.querySelectorAll('[data-screenshot]').forEach(img => {
        const name = img.getAttribute('data-screenshot');
        img.onerror = function() {
            this.onerror = null;
            this.src = `screenshot-${name}.en.png`;
        };
        img.src = `screenshot-${name}.${lang}.png`;
    });

    document.documentElement.lang = lang.replace('_', '-');
    localStorage.setItem('omodsim-lang', lang);
}

document.addEventListener('DOMContentLoaded', () => {
    const dropdown = document.getElementById('langDropdown');
    const btn = document.getElementById('langDropdownBtn');

    btn.addEventListener('click', e => {
        e.stopPropagation();
        const isOpen = dropdown.classList.toggle('open');
        btn.setAttribute('aria-expanded', isOpen);
    });

    document.addEventListener('click', () => {
        if (dropdown.classList.contains('open')) {
            dropdown.classList.remove('open');
            btn.setAttribute('aria-expanded', 'false');
        }
    });

    document.querySelectorAll('.lang-option').forEach(opt => {
        opt.addEventListener('click', e => {
            e.stopPropagation();
            applyTranslations(opt.dataset.lang);
            dropdown.classList.remove('open');
            btn.setAttribute('aria-expanded', 'false');
        });
    });

    const saved = localStorage.getItem('omodsim-lang');
    const lang = (saved && translations[saved]) ? saved : detectBrowserLang();
    applyTranslations(lang);
});
