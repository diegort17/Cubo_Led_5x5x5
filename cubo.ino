// ============================================
// Cubo LED 5x5x5 - Motor con Framebuffer + Animaciones "chingonas"
// ============================================
// Pines 74HC595
const int Lpin  = 8;   // Latch
const int CLpin = 12;  // Clock
const int Dpin  = 11;  // Data
// Capas (Z = 0..4)
int capas[5] = {2, 3, 4, 5, 6};

// ------------ AJUSTES RÁPIDOS (por si tu hardware varía) ------------
// Si tus capas se activan en LOW, cambia a true:
const bool CAPA_ACTIVA_EN_LOW = false;
// Si tus 74HC595 necesitan el orden de bytes inverso (0..3 en vez de 3..0), cambia a true:
const bool INVERTIR_ORDEN_BYTES = false;
// Si tus bits están invertidos dentro de cada byte (LSB↔MSB), cambia a true:
const bool INVERTIR_BITS_EN_BYTE = false;
// --------------------------------------------------------------------

#define NUM_LAYERS 5
#define BYTES_PER_LAYER 4

// Tiempo que se mantiene activa cada capa (controla brillo). 800–2000 µs va bien.
unsigned int REFRESH_US = 1200;

// Framebuffer: para cada capa (z) tenemos 4 bytes (25 bits usados)
byte frame[NUM_LAYERS][BYTES_PER_LAYER];

// --------------- UTILIDADES DE BAJO NIVEL ----------------
inline byte invBits(byte b){
  // invierte el orden de bits b7..b0 -> b0..b7
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

void pushLayerBytes(const byte* bytes){
  digitalWrite(Lpin, LOW);
  if (!INVERTIR_ORDEN_BYTES) {
    for (int reg = BYTES_PER_LAYER - 1; reg >= 0; reg--) {
      byte outB = INVERTIR_BITS_EN_BYTE ? invBits(bytes[reg]) : bytes[reg];
      shiftOut(Dpin, CLpin, MSBFIRST, outB);
    }
  } else {
    for (int reg = 0; reg < BYTES_PER_LAYER; reg++) {
      byte outB = INVERTIR_BITS_EN_BYTE ? invBits(bytes[reg]) : bytes[reg];
      shiftOut(Dpin, CLpin, MSBFIRST, outB);
    }
  }
  digitalWrite(Lpin, HIGH);
}

inline void setCapa(int z, bool on){
  if (CAPA_ACTIVA_EN_LOW) on = !on;
  digitalWrite(capas[z], on ? HIGH : LOW);
}

void apagarTodasCapas(){
  for (int z = 0; z < NUM_LAYERS; z++) setCapa(z, false);
}

// Refresca el frame actual durante "ms" milisegundos (multiplexing estable)
void render(unsigned long ms){
  unsigned long fin = millis() + ms;
  while (millis() < fin) {
    for (int z = 0; z < NUM_LAYERS; z++) {
      // Cargar datos primero
      pushLayerBytes(frame[z]);
      // Encender sólo esta capa
      setCapa(z, true);
      delayMicroseconds(REFRESH_US);
      setCapa(z, false);
    }
  }
}

// --------------- API DE DIBUJO EN EL FRAME ----------------
void clearFrame(){
  for (int z = 0; z < NUM_LAYERS; z++)
    for (int b = 0; b < BYTES_PER_LAYER; b++)
      frame[z][b] = 0;
}

inline void setBit(int z, int col, bool on = true){
  // col 0..24 mapea y(0..4)*5 + x(0..4)
  int reg = col / 8;
  int bit = col % 8;
  if (on) frame[z][reg] |=  (1 << bit);
  else    frame[z][reg] &= ~(1 << bit);
}

inline int idx(int x, int y){ return y * 5 + x; } // x=0..4, y=0..4

void setVoxel(int x, int y, int z, bool on = true){
  if (x < 0 || x > 4 || y < 0 || y > 4 || z < 0 || z > 4) return;
  setBit(z, idx(x, y), on);
}

void fillLayer(int z, bool on = true){
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 5; x++)
      setVoxel(x, y, z, on);
}

void drawWireframeCube(int xmin, int ymin, int zmin, int xmax, int ymax, int zmax){
  // Aristas del cubo (x==xmin/xmax, y==ymin/ymax, z==zmin/zmax)
  for (int x = xmin; x <= xmax; x++)
    for (int y = ymin; y <= ymax; y++) {
      setVoxel(x, y, zmin, true);
      setVoxel(x, y, zmax, true);
    }
  for (int z = zmin; z <= zmax; z++)
    for (int x = xmin; x <= xmax; x++) {
      setVoxel(x, ymin, z, true);
      setVoxel(x, ymax, z, true);
    }
  for (int z = zmin; z <= zmax; z++)
    for (int y = ymin; y <= ymax; y++) {
      setVoxel(xmin, y, z, true);
      setVoxel(xmax, y, z, true);
    }
}

void drawSquarePerimeter2D(int z, int xmin, int ymin, int xmax, int ymax){
  for (int x = xmin; x <= xmax; x++){
    setVoxel(x, ymin, z, true);
    setVoxel(x, ymax, z, true);
  }
  for (int y = ymin; y <= ymax; y++){
    setVoxel(xmin, y, z, true);
    setVoxel(xmax, y, z, true);
  }
}

// --------------- ANIMACIONES "CHINGONAS" ------------------

// 1) Bola 3D rebotando con pequeña cola
void animBolaRebotando(){
  int x=0, y=0, z=0;
  int vx=1, vy=1, vz=1;
  const int cola = 4;
  int qx[cola], qy[cola], qz[cola]; int qn=0;

  for (int t=0; t<120; t++){
    clearFrame();
    // dibuja cola
    for (int i=0; i<qn; i++) setVoxel(qx[i], qy[i], qz[i], true);
    // bola
    setVoxel(x,y,z,true);
    render(35);

    // actualizar cola
    for (int i=cola-1; i>0; i--){ qx[i]=qx[i-1]; qy[i]=qy[i-1]; qz[i]=qz[i-1]; }
    qx[0]=x; qy[0]=y; qz[0]=z; if (qn<cola) qn++;

    // física simple
    x+=vx; y+=vy; z+=vz;
    if (x<=0 || x>=4) vx*=-1, x+=vx;
    if (y<=0 || y>=4) vy*=-1, y+=vy;
    if (z<=0 || z>=4) vz*=-1, z+=vz;
  }
}

// 2) Serpiente 3D con cola flexible
void animSerpiente3D(){
  const int L = 10;
  int sx[L], sy[L], sz[L];
  for (int i=0;i<L;i++){ sx[i]=2; sy[i]=2; sz[i]=2; }
  int dx=1, dy=0, dz=0;
  for (int step=0; step<140; step++){
    // mover cuerpo
    for (int i=L-1; i>0; i--){ sx[i]=sx[i-1]; sy[i]=sy[i-1]; sz[i]=sz[i-1]; }
    // cambiar dirección aleatoriamente (pero evitando quedarse fuera)
    if (random(5)==0){
      int r = random(6);
      int ndx = (r==0)-(r==1);
      int ndy = (r==2)-(r==3);
      int ndz = (r==4)-(r==5);
      if (abs(ndx)+abs(ndy)+abs(ndz)==1){ dx=ndx; dy=ndy; dz=ndz; }
    }
    int nx = sx[0]+dx, ny=sy[0]+dy, nz=sz[0]+dz;
    if (nx<0||nx>4) dx*=-1, nx=sx[0]+dx;
    if (ny<0||ny>4) dy*=-1, ny=sy[0]+dy;
    if (nz<0||nz>4) dz*=-1, nz=sz[0]+dz;
    sx[0]=nx; sy[0]=ny; sz[0]=nz;

    clearFrame();
    for (int i=0;i<L;i++) setVoxel(sx[i],sy[i],sz[i],true);
    render(40);
  }
}

// 3) Cubo de alambre que late (expande / contrae)
void animCuboWireframeLatido(){
  int fases[][6] = {
    {1,1,1,3,3,3}, // cubo pequeño
    {0,0,0,4,4,4}, // cubo grande
    {1,1,1,3,3,3}  // regresa
  };
  for (int rep=0; rep<3; rep++){
    for (int f=0; f<3; f++){
      clearFrame();
      drawWireframeCube(fases[f][0],fases[f][1],fases[f][2],fases[f][3],fases[f][4],fases[f][5]);
      render(250);
    }
  }
}

// 4) Hélice / Corkscrew 3D (trayectoria helicoidal)
void animHelice3D(){
  for (int t=0; t<40; t++){
    clearFrame();
    for (int z=0; z<5; z++){
      int x = (t + 2*z) % 5;
      int y = (t + 3*z) % 5;
      setVoxel(x,y,z,true);
    }
    render(120);
  }
}

// 5) Meteoro con cola que atraviesa el cubo en distintas direcciones
void animMeteoro(){
  int x = random(5), y = random(5), z = 0;
  int vx = (random(2)==0)?1:-1;
  int vy = (random(2)==0)?1:-1;
  int cola = 3;
  int qx[6], qy[6], qz[6], qn=0;

  for (int t=0; t<60; t++){
    clearFrame();
    for (int i=0;i<qn;i++) setVoxel(qx[i],qy[i],qz[i],true);
    setVoxel(x,y,z,true);
    render(45);

    for (int i=5;i>0;i--){ qx[i]=qx[i-1]; qy[i]=qy[i-1]; qz[i]=qz[i-1]; }
    qx[0]=x; qy[0]=y; qz[0]=z; if (qn<cola) qn++;

    x+=vx; y+=vy; z++;
    if (x<=0||x>=4) vx*=-1, x+=vx;
    if (y<=0||y>=4) vy*=-1, y+=vy;
    if (z>4){ z=0; x=random(5); y=random(5); }
  }
}

// 6) Túnel (perímetros decrecientes en Z creando efecto de profundidad)
void animTunel(){
  int sizes[3][4] = {
    {0,0,4,4}, // grande
    {1,1,3,3}, // medio
    {2,2,2,2}  // centro
  };
  for (int rep=0; rep<3; rep++){
    for (int s=0; s<3; s++){
      clearFrame();
      for (int z=0; z<5; z++) drawSquarePerimeter2D(z, sizes[s][0], sizes[s][1], sizes[s][2], sizes[s][3]);
      render(260);
    }
    for (int s=1; s>=0; s--){
      clearFrame();
      for (int z=0; z<5; z++) drawSquarePerimeter2D(z, sizes[s][0], sizes[s][1], sizes[s][2], sizes[s][3]);
      render(260);
    }
  }
}

// 7) Espiral por capas (2D en cada z, avanzando z arriba)
void animEspiralCapas(){
  const int espiral[25] = {
    0,1,2,3,4, 9,14,19,24, 23,22,21,20, 15,10,5, 6,7,8,13,18, 17,16,11,12
  };
  for (int z=0; z<5; z++){
    for (int i=0;i<25;i++){
      clearFrame();
      for (int k=0;k<=i;k++){
        int col = espiral[k];
        setBit(z, col, true);
      }
      render(30);
    }
  }
}

// 8) Twinkle / Estrellas parpadeantes en 3D
void animTwinkle(){
  for (int t=0; t<40; t++){
    clearFrame();
    for (int i=0; i<30; i++){
      setVoxel(random(5), random(5), random(5), true);
    }
    render(120);
  }
}

// ------------------ PLAYLIST ALEATORIA --------------------
typedef void (*AnimFn)();
AnimFn anims[] = {
  animBolaRebotando,
  animSerpiente3D,
  animCuboWireframeLatido,
  animHelice3D,
  animMeteoro,
  animTunel,
  animEspiralCapas,
  animTwinkle
};
const int NUM_ANIMS = sizeof(anims)/sizeof(anims[0]);

// ---------------------------- SETUP / LOOP ----------------------------
void setup(){
  pinMode(Lpin, OUTPUT);
  pinMode(CLpin, OUTPUT);
  pinMode(Dpin, OUTPUT);
  for (int i=0; i<NUM_LAYERS; i++){
    pinMode(capas[i], OUTPUT);
    setCapa(i, false);
  }
  randomSeed(analogRead(A0)); // semilla random
  clearFrame();
}

void loop(){
  // Elige una animación al azar y ejecútala
  int i = random(NUM_ANIMS);
  anims[i]();
  // pequeña pausa negra
  clearFrame(); render(120);
}
