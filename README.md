# Smart Flip Charge - Desafio Duplo: Rotação Contextual e Animação de Carregamento

## 📋 Visão Geral do Projeto

Este projeto consiste em um **desafio duplo de alta complexidade**, dividido em duas partes principais, implementadas no contexto do **Android Open Source Project (AOSP)** e **Lineage OS**:

### 1. **Rotação Automática com Contexto**
Ajuste no comportamento de rotação de tela com base no contexto de reprodução de mídia, permitindo uma rotação automática temporária quando um vídeo está sendo reproduzido, mesmo que a configuração global de rotação automática esteja desativada.

### 2. **Animação de Carregamento Customizada**
Criação de uma animação de carregamento personalizada que mostre, de forma visual e criativa, a porcentagem da bateria enquanto o dispositivo está carregando, incluindo elementos visuais como cabeçalho, versão e uma fonte diferenciada.

---

## 🎯 Objetivos Principais

Melhorar a experiência do usuário em dois aspectos distintos:
- ✅ Comportamento inteligente de rotação de tela contexto-dependente
- ✅ Animação personalizada de carregamento

---

## 🛠️ Tecnologias e Ferramentas Utilizadas

| Tecnologia | Descrição |
|-----------|-----------|
| **Android Open Source Project (AOSP)** | Base do framework Android |
| **Lineage OS** | ROM customizada baseada em AOSP |
| **Emulador Android** | Ambiente de testes virtual |
| **Moto G100** | Dispositivo físico para testes |
| **C++** | Linguagem para animação de carregamento |
| **Biblioteca Minui** | Renderização no framebuffer |
| **MediaSessionManager** | Detecção de reprodução de mídia |
| **AudioManager** | Gerenciamento de áudio e contexto de mídia |
| **Java** | Desenvolvimento de camadas do Framework |

---

## 📁 Estrutura do Projeto

```
smart-flip-charge/
├── new_rotation/
│   ├── RotationButtonController.java
│   └── SystemServer.java
├── system/
│   └── core/
│       └── healthd/
│           ├── healthd_draw.cpp
│           └── healthd_draw.h
└── vendor/
    └── lineage/
        └── charger/
            └── xxhdpi/
                └── percent_font.png
```

---

## 🔧 Detalhes das Alterações Implementadas

### 1. **Rotação Contextual (new_rotation/)**

#### **RotationButtonController.java**
O controlador de rotação foi modificado para adicionar suporte a observadores de configurações em tempo real:

- **Integração de SettingsObserver**: Adicionado um observador (`mSettingsObserver`) que monitora mudanças nas preferências de rotação automática em `Settings.System`
- **Registro Automático**: O observer é registrado tanto no construtor quanto no método `init()`, garantindo que as mudanças sejam detectadas em tempo real
- **Monitoramento de MediaSession**: Integração com `MediaSessionManager` e `AudioManager` para detectar quando vídeos estão sendo reproduzidos
- **Callback de Mudanças**: Método `onChange()` forcado para ler configurações iniciais e responder a mudanças de rotação
- **Logs de Debug**: Adicionados logs informativos para rastreamento do comportamento do observer

```java
// Exemplo das mudanças no construtor:
mSettingsObserver = new SettingsObserver(mMainThreadHandler);
Log.i(TAG, "SettingsObserver: Registrando observer a partir do CONSTRUTOR.");
mSettingsObserver.register();
mSettingsObserver.onChange(false, null);
```

#### **SystemServer.java**
Modificações para suportar a integração do sistema de rotação contextual:

- **Import de Logger**: Adicionado `android.util.Log` para logs do sistema
- **Integração de Serviços**: Preparação da infraestrutura para registrar e gerenciar o serviço de rotação contextual
- **Inicialização de Listeners**: Suporte a listeners de média para detectar estado de reprodução

---

### 2. **Animação de Carregamento Customizada (system/core/healthd/)**

#### **healthd_draw.h**
Arquivo header que define a interface para desenho na tela de carregamento:

- Classe `HealthdDraw` que gerencia renderização no framebuffer
- Métodos para desenho de componentes da interface
- Integração com biblioteca Minui para gráficos de baixo nível
- Suporte a rotação de tela para dispositivos dobráveis

#### **healthd_draw.cpp**
Implementação completa da animação de carregamento com elementos customizados:

##### **Método `draw_header()`**
Desenha o cabeçalho "DevTITANS" no topo da tela:
- Texto: "DevTITANS"
- Posição: 20px + altura da fonte, no topo da tela
- Cor: Verde esmeralda (R:0, G:179, B:13, A:255)
- Altura: Centralizado horizontalmente

```cpp
void HealthdDraw::draw_header(const animation* anim) {
    // Header "DevTITANS" em verde esmeralda
    gr_color(0, 179, 13, 255);
    draw_text(percent_field.font, x, y, header_text.c_str());
}
```

##### **Método `draw_subheader()`**
Desenha subcabeçalho com informações da versão:
- Texto: "$ 2025/1 @"
- Posição: Imediatamente abaixo do header
- Cor: Vermelha Rubi (185, 30, 60)
- Indica versão/release do projeto

```cpp
void HealthdDraw::draw_subheader(const animation* anim) {
    std::string subheader_text = base::StringPrintf("$ 2025/1 @");
    gr_color(185, 30, 60, 255);
}
```

##### **Método `draw_version()`**
Desenha informações da versão do projeto no rodapé da tela:
- Texto: "v2.7.3-final" (ou versão configurada)
- Posição: Rodapé da tela (parte inferior)
- Cor: Azul Safira (20, 90, 200)
- Proporciona identificação visual do build/versão em uso

```cpp
void HealthdDraw::draw_version(const animation* anim) {
    std::string version_text = base::StringPrintf("v2.7.3-final");
    gr_color(20, 90, 200, 255);
    // Posiciona no rodapé da tela
}
```

## 📊 Fluxo de Funcionamento

### Rotação Contextual:
```
MediaSession ativa (vídeo sendo reproduzido)
    ↓
MediaSessionManager detecta mídia
    ↓
SettingsObserver notificado
    ↓
Rotação automática ativada TEMPORARIAMENTE
    ↓
MediaSession finalizada
    ↓
Configuração original de rotação restaurada
```

### Animação de Carregamento:
```
Dispositivo ligado sem bateria suficiente
    ↓
Sistema bootloader carrega healthd
    ↓
healthd_draw renderiza animação
    ↓
Draw Header ("DevTITANS") - Verde Esmeralda
    ↓
Draw Subheader ("$ 2025/1 @") - Vermelho Rubi
    ↓
Draw Percentage (porcentagem) - Variável
    ↓
Draw Version (v2.7.3-beta) - Azul Safira
    ↓
Loop até carregamento suficiente
```

---

## 📦 Arquivos Modificados

### Framework (Java)
| Arquivo | Modificações |
|---------|-------------|
| `new_rotation/RotationButtonController.java` | Adição de SettingsObserver, registro em construtor e init() |
| `new_rotation/SystemServer.java` | Import de Log, preparação para integração de serviços |

### Sistema (C++)
| Arquivo | Modificações |
|---------|-------------|
| `system/core/healthd/healthd_draw.h` | Definição de interface base (sem mudanças significativas) |
| `system/core/healthd/healthd_draw.cpp` | Implementação de novos métodos como: `draw_header()`, `draw_subheader()`, `draw_version()` |

### Assets
| Arquivo | Descrição |
|---------|-----------|
| `vendor/lineage/charger/xxhdpi/percent_font.png` | Fonte customizada manualmente para renderização de percentual, utilizando as fontes 04b_30 e MineDings |

---

## 🎨 Elementos Visuais da Animação

### Layout da Tela de Carregamento

```
┌───────────────────────────────┐
│      DevTITANS (Verde)        │  ← Cabeçalho
│      $ 2025/1 @ (Vermelho)    │  ← Subcabeçalho
│                               │
│                               │
│          XX% (Cor variável)   │  ← Porcentagem
│                               │
│                               │
│      v2.7.3-final (Azul)      │  ← Versão
└───────────────────────────────┘
```

### Paleta de Cores
- **Verde Esmeralda**: `#00B30D` (RGB: 0, 179, 13) - Cabeçalho principal
- **Vermelho Ruby**: `#B91E3C` (RGB: 185, 30, 60) - Cabeçalho secundário
- **Azul Safira**: `#145AC8` (RGB: 20, 90, 200) - Versão

---

## 🔤 Customização Avançada da Fonte (GRFont)

### Descobertas Técnicas Importantes

A customização da fonte para renderização no framebuffer exigiu trabalho manual e detalhado para atender aos requisitos específicos do AOSP/Lineage OS:

#### **1. Formato Esperado pelo GRFont (2 Linhas ASCII)**

A biblioteca Minui do AOSP espera um formato específico de arquivo de fonte chamado **GRFont**, que consiste em:
- **Primeira linha**: Metadados em formato ASCII com informações de largura e altura dos caracteres
- **Segunda linha**: Dados binários da imagem de bitmap contendo os glifos (caracteres)

Exemplo da estrutura:
```
GRFont header (width, height, char_width, char_height)
Binary bitmap data (imagem PNG/RAW com todos os caracteres)
```

#### **2. Resolução do Moto G100**

Após análise comparativa das opções de DPI no vendor do Lineage OS:

| Qualificador de densidade | DPI | Arquivo no Vendor | Status |
|-----|-----------|------------------|--------|
| **hdpi** | 240 | `hdpi/percent_font.png` | ❌ Muito pequeno |
| **xhdpi** | 320 | `xhdpi/percent_font.png` | ❌ Inadequado |
| **xxhdpi** | 480 | `xxhdpi/percent_font.png` | ✅ **UTILIZADO** |
| **xxxhdpi** | 640 | `xxxhdpi/percent_font.png` | ❌ Muito grande |

O Moto G100 opera na densidade **xxhdpi (480 dpi)**, fornecendo resolução adequada para renderizar texto de alta qualidade durante a animação de carregamento.

**Comando para verificar DPI do device:**
```bash
adb shell wm density
# Output esperado: Physical density: 480
```

#### **3. Processo de Customização Manual com GIMP**

O arquivo `percent_font.png` foi customizado manualmente usando GIMP para:

1. **Criar layout de caracteres**: Organizar todos os caracteres (0-9, %, etc.) em uma grid
2. **Definir dimensões precisas**: A partir dos poucos caracteres originais, mapear espaço para outros caracteres ASCII (símbolos e alfabeto maísculo e minúsculo)
3. **Aplicar fontes visuais**: 
   - Fonte primária: **04b_30** (bitmap clássica do AOSP)
   - Fonte secundária: **MineDings** (símbolos especiais)
4. **Otimizar para framebuffer**:
   - Converter para RGB conforme esperado pelo minui, pois ARGB não é reconhecido
   - Garantir contraste máximo (branco sobre preto ou vice-versa)
   - Validar ausência de artefatos de compressão (PNG lossless)

**Passos técnicos no GIMP:**
```
1. Arquivo → Novo (xxhdpi, RGB)
2. Importar fontes 04b_30 e MineDings
3. Exibir grid que que perfeitamente engloba todos os caracteres com mesma margem
3.1 (Opcional) Adicionar duas linhas guias horizontais a 25% e 75% de altura
4. Renderizar textos para células específicas
5. Mesclar camadas
6. Exportar como PNG (sem compressão/interlace)
7. Validar tamanho final (deve ter exatamente as dimensões esperadas, neste caso 6912x522 pixels)
```

#### **4. Validação Final da Fonte Customizada**

Antes de usar a fonte no build, foram realizadas validações:

```bash
# Verificar dimensões da imagem
file vendor/lineage/charger/xxhdpi/percent_font.png
# Output: 6912x522 pixels, 8-bit/color RGB

# Verificar se arquivo é válido PNG
pngcheck -v vendor/lineage/charger/xxhdpi/percent_font.png
```

#### **5. Relacionamento com healthd_draw.cpp**

No código C++, a fonte é carregada assim:

```cpp
if (!anim->text_percent.font_file.empty() &&
    (res = gr_init_font(anim->text_percent.font_file.c_str(), 
                        &anim->text_percent.font)) < 0) {
    LOGE("Could not load percent font (%d)\n", res);
}
```

O arquivo PNG customizado é referenciado como `anim->text_percent.font_file` e carregado pela função `gr_init_font()` do Minui, que:
1. Lê o header GRFont da primeira linha
2. Decodifica o bitmap PNG
3. Cria estrutura GRFont com mapa de caracteres
4. Permite renderização via `gr_text(font, x, y, str, ...)`

---

## 🧪 Testes e Validação

### Dispositivos Testados
- ✅ **Emulador Android**: Testes de integração básica
- ✅ **Moto G100**: Validação em hardware real

### Cenários de Teste
1. **Rotação Contextual**
   - [ ] Reprodução de vídeo sem rotação automática global ativada
   - [ ] Mudança de contexto durante rotação
   - [ ] Restauração de configuração original

2. **Animação de Carregamento**
   - [ ] Exibição correta de textos, fontes e cores

---

## 👥 Membros do Time

| Nome | Cargo | Responsabilidade |
|------|-------|-----------------|
| Gabriel Isaac Gonçalves Haydar | Engenheiro de Build e Ambiente e Engenheiro de Animação (C++ / Gráficos) | Criar o ambiente de desenvolvimento e garantir um fluxo de trabalho rápido, e criar a animação de carregamento em C++ |
| Luan Albuquerque dos Santos | Engenheiro de Animação (C++ / Gráficos) | Criar a animação de carregamento em C++ |
| Suelen da Silva Pereira | Engenheiro de Animação (C++ / Gráficos) | Criar a animação de carregamento em C++ |
| Nilton da Silva Nascimento | Arquiteto de Framework (Líder da Sub-equipe Rotação) | Projetar a solução de detecção de vídeo e definir a arquitetura da rotação contextual |
| Alexandre Bruno Mota dos Santos | Engenheiro de Serviço de Sistema (Implementação Rotação) | Implementar os mecanismos de detecção de vídeo |
| Hellmut Albert Alenca Schuster | Engenheiro de Integração e Testes Finais | Unificar as soluções de animação e rotação, além de validar a solução completa |

---

## 📚 Referências Técnicas

### AOSP Documentation
- [WindowManager API](https://developer.android.com/reference/android/view/WindowManager)
- [RotationPolicy](https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/core/java/com/android/internal/view/RotationPolicy.java)
- [MediaSessionManager](https://developer.android.com/reference/android/media/session/MediaSessionManager)

### Lineage OS
- [Lineage OS Project](https://lineageos.org/)
- [Charger Animation Documentation](https://wiki.lineageos.org/)

### Minui
- [Minui Graphics Library](https://android.googlesource.com/platform/system/core/+/refs/heads/main/minui/)

---

## ⚙️ Compilação e Deploy

### Build AOSP
```bash
# Configurar ambiente
source build/envsetup.sh

# Compilar para dispositivo específico
breakfast nio
brunch nio
```

### Flash em Dispositivo
```bash
adb reboot recovery
adb sideload out/target/product/nio/lineage-21.0-YYYYmmDD-UNOFFICIAL-nio.zip
```

---

## 🐛 Resolução de Problemas

### Animação não aparece
- Verificar se `healthd` está sendo executado corretamente
- Verificar se texto é grande demais para tela
- Validar caminho do arquivo de fonte em `percent_font.png`, que deve ser substituído na resolução xxhdpi, no MOTO G100

### Rotação contextual não funciona
- Verificar se `MediaSessionManager` está registrado
- Validar permissões de acesso a `Settings.System`
- Conferir logs: `adb logcat | grep RotationButton`

---

## 📝 Notas de Desenvolvimento

- Este projeto requer compilação customizada do AOSP/Lineage OS
- Mudanças em `healthd_draw.cpp` afetam a tela de boot - testar sempre em hardware real
- O SettingsObserver deve ser registrado ANTES de chamar observadores para evitar race conditions
- Cores são definidas como ARGB8888 (RGB + Alpha)

---

## 📄 Licença

Este projeto segue a licença Apache License 2.0, conforme padrão do AOSP.

```
Copyright (C) 2025 DevTITANS

Licensed under the Apache License, Version 2.0
```

---

**Turma**: 2025/1  
**Última Atualização**: 26 de novembro de 2025  
**Status**: Concluído

