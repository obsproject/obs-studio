import SwiftUI

private enum ObsStyle {
    static let background = Color(red: 27.0 / 255, green: 28.0 / 255, blue: 32.0 / 255)
    static let dock = Color(red: 37.0 / 255, green: 39.0 / 255, blue: 46.0 / 255)
    static let dockHeader = Color(red: 48.0 / 255, green: 50.0 / 255, blue: 58.0 / 255)
    static let raised = Color(red: 65.0 / 255, green: 67.0 / 255, blue: 76.0 / 255)
    static let muted = Color(red: 190.0 / 255, green: 192.0 / 255, blue: 198.0 / 255)
    static let blue = Color(red: 49.0 / 255, green: 89.0 / 255, blue: 183.0 / 255)
    static let green = Color(red: 56.0 / 255, green: 179.0 / 255, blue: 109.0 / 255)
}

struct ObsMobileView: View {
    @State private var scenes = ["abertura", "pc-1"]
    @State private var selectedScene = 0
    @State private var sourceCollections: [[String]] = [[], []]
    @State private var selectedSource: Int? = nil
    @State private var hiddenSources = Set<Int>()
    @State private var showingSceneName = false
    @State private var showingSourcePicker = false
    @State private var newName = ""
    @State private var mutedMic = false
    @State private var mutedDesktop = false
    @State private var gain = 0.0
    @State private var transition = "Esmaecer"
    @State private var transitionDuration = 300.0
    @State private var studioMode = false
    @State private var notice: String?
    private var sources: [String] {
        get { sourceCollections.indices.contains(selectedScene) ? sourceCollections[selectedScene] : [] }
        set { if sourceCollections.indices.contains(selectedScene) { sourceCollections[selectedScene] = newValue } }
    }

    var body: some View {
        GeometryReader { geometry in
            Group {
                if geometry.size.width >= 760 && geometry.size.width > geometry.size.height {
                    desktopLayout
                } else {
                    portraitLayout
                }
            }
            .background(ObsStyle.background.ignoresSafeArea())
        }
        .preferredColorScheme(.dark)
        .alert("OBS Studio Mobile", isPresented: Binding(get: { notice != nil }, set: { if !$0 { notice = nil } })) {
            Button("OK") { notice = nil }
        } message: { Text(notice ?? "") }
        .alert("Nova cena", isPresented: $showingSceneName) {
            TextField("Nome da cena", text: $newName)
            Button("Cancelar", role: .cancel) { newName = "" }
            Button("Adicionar") { if !newName.trimmingCharacters(in: .whitespaces).isEmpty { scenes.append(newName); sourceCollections.append([]); selectedScene = scenes.count - 1; selectedSource = nil; newName = "" } }
        }
        .confirmationDialog("Adicionar fonte", isPresented: $showingSourcePicker, titleVisibility: .visible) {
            ForEach(["Dispositivo de captura de vídeo", "Captura de tela", "Captura de janela", "Texto", "Imagem", "Captura de áudio"], id: \.self) { item in
                Button(item) { sources.append(item); selectedSource = sources.count - 1 }
            }
        } message: { Text("Adiciona uma fonte à cena atual. A captura real requer a integração nativa.") }
    }

    private var portraitLayout: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 2) {
                titleBar
                menuBar
                previewDock(desktop: false)
                    scenesDock
                    sourcesDock
                mixerDock
                transitionDock
                controlsDock
                statusBar
            }
            .padding(.bottom, 8)
        }
        .background(ObsStyle.background)
    }

    private var desktopLayout: some View {
        VStack(spacing: 2) {
            titleBar
            menuBar
            HStack(spacing: 3) {
                VStack(spacing: 3) {
                scenesDock.frame(maxHeight: .infinity).background(ObsStyle.dock)
                    sourcesDock.frame(maxHeight: .infinity).background(ObsStyle.dock)
                }
                .frame(width: 185)

                VStack(spacing: 3) {
                    previewDock(desktop: true).frame(maxHeight: .infinity).background(ObsStyle.dock)
                    HStack(spacing: 3) {
                        mixerDock.frame(maxWidth: .infinity, maxHeight: .infinity).background(ObsStyle.dock)
                        transitionDock.frame(maxWidth: .infinity, maxHeight: .infinity).background(ObsStyle.dock)
                        controlsDock.frame(maxWidth: .infinity, maxHeight: .infinity).background(ObsStyle.dock)
                    }
                    .frame(height: 195)
                }
            }
            .padding(.horizontal, 2)
            statusBar
        }
        .background(ObsStyle.background)
    }

    private var menuBar: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 15) {
                ForEach(["Arquivo", "Editar", "Visualizar", "Painéis", "Perfil", "Coleção de cenas", "Ferramentas", "Ajuda", "Streamlabs"], id: \.self) { item in
                    Text(item).foregroundStyle(ObsStyle.muted)
                }
            }
            .font(.system(size: 11))
            .padding(.horizontal, 8)
        }
        .frame(height: 35)
        .background(Color(red: 29.0 / 255, green: 30.0 / 255, blue: 34.0 / 255))
    }

    private var titleBar: some View {
        HStack(spacing: 7) {
            Text("◉")
            Text("OBS 33.0.1 - Perfil: Sem nome - Cenas: Sem nome").lineLimit(1)
            Spacer(minLength: 0)
        }
        .font(.system(size: 10))
        .foregroundStyle(.white)
        .padding(.horizontal, 8)
        .frame(height: 29)
        .background(Color(red: 29.0 / 255, green: 30.0 / 255, blue: 34.0 / 255))
    }

    private func panel<Content: View>(_ title: String, @ViewBuilder content: () -> Content) -> some View {
        VStack(spacing: 0) {
            HStack {
                Text(title).font(.system(size: 10, weight: .bold)).foregroundStyle(.white)
                Spacer()
                Image(systemName: "rectangle.on.rectangle").font(.system(size: 9)).foregroundStyle(ObsStyle.muted)
            }
            .padding(.horizontal, 8)
            .frame(height: 25)
            .background(ObsStyle.dockHeader)
            content()
        }
        .frame(maxWidth: .infinity, alignment: .topLeading)
        .background(ObsStyle.dock)
    }

    private var scenesDock: some View {
        panel("Cenas") {
            VStack(spacing: 2) {
                ForEach(scenes.indices, id: \.self) { index in
                    Button {
                        selectedScene = index
                        selectedSource = nil
                    } label: {
                        HStack {
                            Text(scenes[index]).font(.system(size: 12))
                            Spacer()
                        }
                        .foregroundStyle(.white)
                        .padding(.horizontal, 7)
                        .frame(height: 29)
                        .background(index == selectedScene ? ObsStyle.blue : .clear, in: RoundedRectangle(cornerRadius: 3))
                    }
                    .buttonStyle(.plain)
                }
                Spacer(minLength: 4)
                HStack(spacing: 17) {
                    Button("＋") {
                        showingSceneName = true
                    }
                    .foregroundStyle(.white)
                    Button { if scenes.count > 1 { scenes.remove(at: selectedScene); sourceCollections.remove(at: selectedScene); selectedScene = max(0, selectedScene - 1); selectedSource = nil } } label: { Image(systemName: "trash") }.foregroundStyle(ObsStyle.muted).disabled(scenes.count <= 1)
                    Image(systemName: "rectangle.on.rectangle").foregroundStyle(ObsStyle.muted)
                    Button { if selectedScene > 0 { scenes.swapAt(selectedScene, selectedScene - 1); sourceCollections.swapAt(selectedScene, selectedScene - 1); selectedScene -= 1 } } label: { Image(systemName: "chevron.up") }.foregroundStyle(ObsStyle.muted)
                    Button { if selectedScene + 1 < scenes.count { scenes.swapAt(selectedScene, selectedScene + 1); sourceCollections.swapAt(selectedScene, selectedScene + 1); selectedScene += 1 } } label: { Image(systemName: "chevron.down") }.foregroundStyle(ObsStyle.muted)
                }
                .font(.system(size: 13))
                .frame(maxWidth: .infinity, minHeight: 33, alignment: .leading)
                .padding(.horizontal, 10)
            }
            .padding(.top, 3)
            .padding(.horizontal, 3)
        }
    }

    private var sourcesDock: some View {
        panel("Fontes") {
            VStack {
                if sources.isEmpty {
                    Spacer(minLength: 5)
                    Text("ⓘ\nClique em + para adicionar uma fonte").multilineTextAlignment(.center).font(.system(size: 10)).foregroundStyle(ObsStyle.muted)
                    Spacer(minLength: 5)
                } else {
                    ForEach(sources.indices, id: \.self) { index in
                        HStack {
                            Button { if hiddenSources.contains(index) { hiddenSources.remove(index) } else { hiddenSources.insert(index) } } label: { Image(systemName: hiddenSources.contains(index) ? "eye.slash" : "eye") }
                            Button { selectedSource = index } label: { HStack { Text(sources[index]); Spacer(); if selectedSource == index { Image(systemName: "checkmark") } } }.buttonStyle(.plain)
                        }
                        .font(.system(size: 10)).foregroundStyle(.white).padding(6).background(selectedSource == index ? ObsStyle.blue : .clear)
                    }
                }
                Spacer(minLength: 5)
                HStack(spacing: 17) {
                    Button { showingSourcePicker = true } label: { Text("＋") }.foregroundStyle(.white)
                    Button { if let index = selectedSource, sources.indices.contains(index) { sources.remove(at: index); selectedSource = nil } } label: { Image(systemName: "trash") }.disabled(selectedSource == nil)
                    Button { if let index = selectedSource, index > 0 { sources.swapAt(index, index - 1); selectedSource = index - 1 } } label: { Image(systemName: "chevron.up") }.disabled((selectedSource ?? 0) == 0)
                    Button { if let index = selectedSource, index + 1 < sources.count { sources.swapAt(index, index + 1); selectedSource = index + 1 } } label: { Image(systemName: "chevron.down") }.disabled((selectedSource ?? 0) + 1 >= sources.count)
                }
                .font(.system(size: 13))
                .foregroundStyle(ObsStyle.muted)
                .frame(maxWidth: .infinity, minHeight: 33, alignment: .leading)
                .padding(.horizontal, 10)
            }
        }
    }

    private func previewDock(desktop: Bool) -> some View {
        VStack(spacing: 0) {
            if desktop {
                Color.black.frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                Color.black.aspectRatio(16.0 / 9.0, contentMode: .fit).frame(maxWidth: .infinity)
            }
            .padding(8)
            HStack(spacing: 8) {
                Text("−     42%     +").padding(.horizontal, 6)
                Text("Dimensionar à janela   ▾")
                Spacer()
            }
            .font(.system(size: 10))
            .foregroundStyle(ObsStyle.muted)
            .frame(height: 27)
            .background(ObsStyle.dock)

            HStack(spacing: 8) {
                Text("Nenhuma fonte selecionada").foregroundStyle(.white)
                Spacer()
                Label("Propriedades", systemImage: "gearshape.fill")
                Label("Filtros", systemImage: "slider.horizontal.3")
            }
            .font(.system(size: 10))
            .foregroundStyle(ObsStyle.muted)
            .padding(.horizontal, 8)
            .frame(height: 40)
            .background(ObsStyle.dockHeader)
        }
        .background(Color(red: 20.0 / 255, green: 21.0 / 255, blue: 25.0 / 255))
    }

    private var mixerDock: some View {
        panel("Mixer de áudio") {
            HStack(alignment: .top, spacing: 5) {
                audioChannel("Mic/Aux", muted: $mutedMic)
                audioChannel("Áudio desktop", muted: $mutedDesktop)
            }
            .padding(6)
            Spacer(minLength: 0)
            Text("0 oculto       ⚙   Opções ▾")
                .font(.system(size: 9))
                .foregroundStyle(ObsStyle.muted)
                .frame(maxWidth: .infinity, minHeight: 25, alignment: .leading)
                .padding(.horizontal, 7)
        }
    }

    private func audioChannel(_ name: String, muted: Binding<Bool>) -> some View {
        VStack(spacing: 4) {
            Text("Global").font(.system(size: 8, weight: .bold)).foregroundStyle(.white)
                .frame(maxWidth: .infinity).padding(.vertical, 3).background(ObsStyle.blue)
            Text(name).font(.system(size: 9)).lineLimit(1).foregroundStyle(.white)
            Text(String(format: "%.1f dB", gain)).font(.system(size: 8)).foregroundStyle(ObsStyle.muted)
            Slider(value: $gain, in: -60...12).tint(ObsStyle.blue)
            HStack(alignment: .bottom, spacing: 3) {
                VStack(spacing: 1) {
                    ForEach(0..<12, id: \.self) { index in
                        Rectangle().fill(index > 9 ? Color.red : (index > 7 ? Color.yellow : ObsStyle.green)).frame(width: 8, height: 5)
                    }
                }
                VStack(alignment: .leading, spacing: 0) {
                    ForEach(["-6", "-12", "-18", "-24", "-30", "-36", "-42", "-48", "-54", "-60"], id: \.self) {
                        Text($0).font(.system(size: 7)).foregroundStyle(ObsStyle.muted)
                    }
                }
            }
            HStack {
                Button { muted.wrappedValue.toggle() } label: { Image(systemName: muted.wrappedValue ? "speaker.slash.fill" : "speaker.wave.2.fill") }
                Button { gain = 0 } label: { Image(systemName: "arrow.counterclockwise") }
            }.font(.system(size: 10)).foregroundStyle(.white)
        }
        .frame(width: 60)
        .padding(3)
    }

    private var transitionDock: some View {
        panel("Transições de cena") {
            VStack(alignment: .leading, spacing: 9) {
                Menu {
                    ForEach(["Esmaecer", "Corte", "Esmaecer para preto"], id: \.self) { item in Button(item) { transition = item } }
                } label: {
                    HStack { Text(transition); Spacer(); Text("▾") }.font(.system(size: 10)).foregroundStyle(.white).padding(8).background(ObsStyle.raised, in: RoundedRectangle(cornerRadius: 4))
                }
                HStack {
                    Text("Duração: \(Int(transitionDuration)) ms").foregroundStyle(.white)
                }
                .font(.system(size: 9))
                Slider(value: $transitionDuration, in: 0...2000, step: 50).tint(ObsStyle.blue)
                HStack(spacing: 6) {
                    Text("Corte")
                    Text("Esmaecer para preto")
                }
                .font(.system(size: 8))
                .foregroundStyle(ObsStyle.muted)
            }
            .padding(7)
            Spacer(minLength: 0)
        }
    }

    private var controlsDock: some View {
        panel("Controles") {
            VStack(spacing: 4) {
                ForEach(["Iniciar transmissão", "Gerenciar transmissão", "Iniciar gravação", "Iniciar câmera virtual", "Modo estúdio", "Configurações"], id: \.self) { item in
                    Button {
                        switch item {
                        case "Modo estúdio": studioMode.toggle()
                        case "Iniciar gravação": notice = "Gravação ainda não conectada: falta integrar libobs e o encoder nativo."
                        case "Iniciar transmissão", "Gerenciar transmissão": notice = "Transmissão ainda não conectada: falta integrar encoder e saída RTMP."
                        case "Iniciar câmera virtual": notice = "Câmera virtual não disponível no iOS/Android sem integração nativa específica."
                        default: notice = "Configurações do motor OBS ainda não integradas."
                        }
                    } label: { Text(item == "Modo estúdio" && studioMode ? "Sair do modo estúdio" : item)
                        .font(.system(size: 9))
                        .foregroundStyle(ObsStyle.muted)
                        .frame(maxWidth: .infinity, minHeight: 27)
                        .background(ObsStyle.raised, in: RoundedRectangle(cornerRadius: 4))
                    }.buttonStyle(.plain)
                }
            }
            .padding(5)
            Spacer(minLength: 0)
        }
    }

    private var statusBar: some View {
        HStack(spacing: 9) {
            Text("▮▮")
            Text("◉ 00:00:00")
            Text("● 00:00:00")
            Spacer(minLength: 2)
            Text("CPU: 0.0%")
            Text("30.00 / 30.00 FPS")
        }
        .font(.system(size: 9))
        .foregroundStyle(ObsStyle.muted)
        .padding(.horizontal, 8)
        .frame(height: 30)
        .background(Color(red: 29.0 / 255, green: 30.0 / 255, blue: 34.0 / 255))
    }
}
