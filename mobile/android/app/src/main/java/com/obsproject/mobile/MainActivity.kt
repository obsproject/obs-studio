package com.obsproject.mobile

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

private val canvas = Color(0xFF1B1C20); private val dock = Color(0xFF25272E)
private val header = Color(0xFF30323A); private val raised = Color(0xFF41434C)
private val muted = Color(0xFFBEC0C6); private val blue = Color(0xFF3159B7)
private class Scene(val name: String, val sources: MutableList<String> = mutableStateListOf())

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) { super.onCreate(savedInstanceState); setContent { MaterialTheme { ObsApp() } } }
}

@Composable private fun ObsApp() {
    val scenes = remember { mutableStateListOf(Scene("abertura"), Scene("pc-1")) }
    var selected by remember { mutableIntStateOf(0) }; var selectedSource by remember { mutableIntStateOf(-1) }
    var dialog by remember { mutableStateOf("") }; var input by remember { mutableStateOf("") }
    var mutedMic by remember { mutableStateOf(false) }; var mutedDesktop by remember { mutableStateOf(false) }
    var gain by remember { mutableFloatStateOf(0f) }; var duration by remember { mutableIntStateOf(300) }
    var transition by remember { mutableStateOf("Esmaecer") }; var studio by remember { mutableStateOf(false) }
    var status by remember { mutableStateOf("Pronto") }
    val current = scenes.getOrNull(selected)
    fun notify(text: String) { status = text; dialog = text }
    if (dialog.isNotEmpty()) AlertDialog(onDismissRequest = { dialog = "" }, title = { Text(if (dialog == "scene" || dialog == "source") "Adicionar" else "OBS Mobile") },
        text = { if (dialog == "scene" || dialog == "source") Column {
            if (dialog == "source") listOf("Dispositivo de captura de vídeo", "Captura de tela", "Captura de janela", "Texto", "Imagem", "Captura de áudio").forEach { TextButton(onClick = { current?.sources?.add(it); selectedSource = (current?.sources?.size ?: 1)-1; dialog = "" }) { Text(it) } }
            OutlinedTextField(input, { input = it }, label = { Text(if (dialog == "scene") "Nome da cena" else "Nome da fonte") })
        } else Text(dialog) },
        confirmButton = { if (dialog == "scene" || dialog == "source") TextButton(onClick = { if (input.isNotBlank()) { if (dialog == "scene") { scenes.add(Scene(input)); selected = scenes.lastIndex } else { current?.sources?.add(input); selectedSource = (current?.sources?.size ?: 1)-1 }; input = ""; dialog = "" } }) { Text("Criar") } else TextButton(onClick = { dialog = "" }) { Text("OK") } },
        dismissButton = { if (dialog == "scene" || dialog == "source") TextButton(onClick = { dialog = "" }) { Text("Cancelar") } })
    Surface(Modifier.fillMaxSize(), color = canvas) {
        BoxWithConstraints {
            val landscape = maxWidth >= 760.dp && maxWidth > maxHeight
            if (landscape) Column(Modifier.fillMaxSize()) {
                Title(); Menus()
                Row(Modifier.weight(1f).padding(3.dp), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
                    Column(Modifier.weight(.8f), verticalArrangement = Arrangement.spacedBy(3.dp)) {
                        ScenePanel(scenes, selected, { selected = it; selectedSource = -1 }, { dialog = "scene" }, { if (scenes.size > 1) { scenes.removeAt(selected); selected = 0; selectedSource = -1 } }, { target -> scenes.add(target, scenes.removeAt(selected)); selected = target }, Modifier.weight(1.1f))
                        SourcePanel(current, selectedSource, { selectedSource = it }, { dialog = "source" }, { if (selectedSource in (current?.sources?.indices ?: IntRange.EMPTY)) { current?.sources?.removeAt(selectedSource); selectedSource = -1 } }, { target -> current?.sources?.let { it.add(target, it.removeAt(selectedSource)); selectedSource = target } }, Modifier.weight(.9f))
                    }
                    Column(Modifier.weight(2.8f), verticalArrangement = Arrangement.spacedBy(3.dp)) {
                        Preview(Modifier.weight(1.5f), studio)
                        Row(Modifier.weight(1f), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
                            Mixer(Modifier.weight(1.3f), mutedMic, { mutedMic = !mutedMic }, mutedDesktop, { mutedDesktop = !mutedDesktop }, gain) { gain = it }
                            Transition(Modifier.weight(.9f), transition, { transition = it }, duration, { duration = it })
                            Controls(Modifier.weight(1.1f), studio, { studio = !studio }, { notify("Configurações do motor OBS ainda não integradas") }, { notify("A gravação exige o motor OBS e encoder nativo") }, { notify("Transmissão exige encoder e configuração RTMP") })
                        }
                    }
                }
                Status(status)
            } else LazyColumn(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(2.dp)) {
                item { Title() }; item { Menus() }; item { Preview(Modifier, studio) }
                item { ScenePanel(scenes, selected, { selected = it; selectedSource = -1 }, { dialog = "scene" }, { if (scenes.size > 1) { scenes.removeAt(selected); selected = 0 } }, { target -> scenes.add(target, scenes.removeAt(selected)); selected = target }) }
                item { SourcePanel(current, selectedSource, { selectedSource = it }, { dialog = "source" }, { if (selectedSource >= 0) current?.sources?.removeAt(selectedSource) }, { target -> current?.sources?.let { it.add(target, it.removeAt(selectedSource)); selectedSource = target } }) }
                item { Mixer(Modifier, mutedMic, { mutedMic = !mutedMic }, mutedDesktop, { mutedDesktop = !mutedDesktop }, gain, { gain = it }) }
                item { Transition(Modifier, transition, { transition = it }, duration, { duration = it }) }
                item { Controls(Modifier, studio, { studio = !studio }, { notify("Configurações do motor OBS ainda não integradas") }, { notify("A gravação exige o motor OBS e encoder nativo") }, { notify("Transmissão exige encoder e configuração RTMP") }) }
                item { Status(status) }
            }
        }
    }
}

@Composable private fun Title() { Text("◉  OBS Studio Mobile — Perfil: Sem nome", Modifier.fillMaxWidth().background(Color(0xFF1D1E22)).padding(8.dp), Color.White, fontSize=11.sp) }
@Composable private fun Menus() { Row(Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).background(Color(0xFF1D1E22)).padding(8.dp), horizontalArrangement=Arrangement.spacedBy(15.dp)) { listOf("Arquivo","Editar","Visualizar","Painéis","Perfil","Coleção de cenas","Ferramentas","Ajuda").forEach { Text(it, color=muted, fontSize=11.sp) } } }
@Composable private fun Panel(title: String, modifier: Modifier=Modifier, content: @Composable ColumnScope.()->Unit) { Column(modifier.fillMaxWidth().background(dock)) { Text(title, Modifier.fillMaxWidth().background(header).padding(6.dp), Color.White, fontSize=10.sp, fontWeight=FontWeight.Bold); content() } }
@Composable private fun ScenePanel(scenes: List<Scene>, selected: Int, select:(Int)->Unit, add:()->Unit, delete:()->Unit, move:(Int)->Unit, modifier:Modifier=Modifier) {
    Panel("Cenas", modifier) { scenes.forEachIndexed { i,s -> Text(s.name, Modifier.fillMaxWidth().padding(2.dp).background(if(i==selected) blue else Color.Transparent, RoundedCornerShape(3.dp)).clickable { select(i) }.padding(7.dp), Color.White, fontSize=12.sp) }
        Row { TextButton(onClick=add) { Text("＋") }; TextButton(onClick=delete, enabled=scenes.size>1) { Text("▤") }; TextButton(onClick={ if(selected>0) move(selected-1) }, enabled=selected>0) { Text("↑") }; TextButton(onClick={ if(selected<scenes.lastIndex) move(selected+1) }, enabled=selected<scenes.lastIndex) { Text("↓") } }
    }
}
@Composable private fun SourcePanel(scene: Scene?, selected:Int, select:(Int)->Unit, add:()->Unit, delete:()->Unit, move:(Int)->Unit, modifier:Modifier=Modifier) {
    Panel("Fontes", modifier) { if(scene?.sources.isNullOrEmpty()) Text("ⓘ  Clique em + para adicionar uma fonte", Modifier.padding(12.dp), muted, fontSize=10.sp)
        scene?.sources?.forEachIndexed { i,s -> var visible by remember(scene,i) { mutableStateOf(true) }; Row(Modifier.fillMaxWidth().clickable { select(i) }.background(if(i==selected) blue else Color.Transparent).padding(horizontal=8.dp, vertical=5.dp), verticalAlignment=Alignment.CenterVertically) { Text(if(visible) "◉" else "⊘", Modifier.clickable { visible=!visible }, Color.White); Text("  $s", Modifier.weight(1f), Color.White, fontSize=10.sp); Text("☷", color=muted) } }
        Row { TextButton(onClick=add) { Text("＋") }; TextButton(onClick=delete, enabled=selected>=0) { Text("▤") }; TextButton(onClick={ if(selected>0) move(selected-1) }, enabled=selected>0) { Text("↑") }; TextButton(onClick={ if(selected < (scene?.sources?.lastIndex ?: -1)) move(selected+1) }, enabled=selected < (scene?.sources?.lastIndex ?: -1)) { Text("↓") } }
    }
}
@Composable private fun Preview(modifier:Modifier, studio:Boolean) { Column(modifier.fillMaxWidth().background(Color(0xFF141519)).padding(6.dp)) { Box(Modifier.weight(1f, fill=false).fillMaxWidth().aspectRatio(16f/9f).background(Color.Black), Alignment.Center) { if(studio) Text("MODO ESTÚDIO", color=muted) }; Row(Modifier.fillMaxWidth().background(dock).padding(7.dp)) { Text("−   42%   +     Dimensionar à janela", color=muted, fontSize=10.sp) }; Text("Nenhuma fonte selecionada       ⚙ Propriedades       ▤ Filtros", Modifier.fillMaxWidth().background(header).padding(10.dp), Color.White, fontSize=10.sp) } }
@Composable private fun Mixer(modifier:Modifier, mic:Boolean, toggleMic:()->Unit, desktop:Boolean, toggleDesktop:()->Unit, gain:Float, setGain:(Float)->Unit={}) { Panel("Mixer de áudio",modifier) { Row(Modifier.padding(6.dp)) { listOf("Mic/Aux" to mic,"Áudio desktop" to desktop).forEach { (name,isMuted) -> Column(Modifier.weight(1f).background(header).padding(5.dp)) { Text(name,color=Color.White,fontSize=10.sp); Text("0.0 dB     ▂▅▇",color=Color.Green,fontSize=9.sp); Slider(value=gain,onValueChange=setGain,valueRange=-60f..12f); TextButton(onClick=if(name=="Mic/Aux") toggleMic else toggleDesktop) { Text(if(isMuted) "Som ativado" else "Silenciar") } } } } } }
@Composable private fun Transition(modifier:Modifier, value:String, set:(String)->Unit, duration:Int, setDuration:(Int)->Unit) { Panel("Transições de cena",modifier) { Row(Modifier.padding(8.dp), verticalAlignment=Alignment.CenterVertically) { listOf("Esmaecer","Corte","Esmaecer para preto").forEach { TextButton(onClick={set(it)}) { Text(if(value==it) "✓ $it" else it, fontSize=9.sp) } } }; Row(verticalAlignment=Alignment.CenterVertically) { Text("Duração: ${duration} ms", color=Color.White, fontSize=10.sp); Slider(value=duration.toFloat(),onValueChange={setDuration(it.toInt())},valueRange=0f..2000f) } } }
@Composable private fun Controls(modifier:Modifier, studio:Boolean, toggleStudio:()->Unit, settings:()->Unit, record:()->Unit, stream:()->Unit) { Panel("Controles",modifier) { listOf("Iniciar transmissão" to stream,"Gerenciar transmissão" to settings,"Iniciar gravação" to record,"Iniciar câmera virtual" to settings,(if(studio) "Sair do modo estúdio" else "Modo estúdio") to toggleStudio,"Configurações" to settings).forEach { (label,action) -> TextButton(onClick=action, modifier=Modifier.fillMaxWidth().padding(horizontal=5.dp)) { Text(label, color=Color.White, fontSize=10.sp) } } } }
@Composable private fun Status(text:String) { Text("● 00:00:00     $text                                      CPU: —     30 / 30 FPS", Modifier.fillMaxWidth().background(Color(0xFF1D1E22)).padding(8.dp), muted, fontSize=9.sp) }
