# UAI Station

UAI Station é o backend de um projeto de console retrô. Ele usa emuladores por baixo dos panos para rodar jogos clássicos e expõe uma camada simples de execução para o restante do sistema.

O projeto é focado em manter o núcleo pequeno e fácil de expandir, com implementações separadas de emuladores e suporte a múltiplas plataformas retrô.

## Supported Platforms

- CHIP-8
- PlayStation 1

## Estrutura do Projeto

- `src/` - código da aplicação e das implementações dos emuladores
- `include/` - headers públicos e interfaces compartilhadas
- `assets/` - arquivos de BIOS e conteúdo dos jogos
- `cores/` - binários dos cores libretro

## Observações

- O suporte a PS1 depende de um core libretro compatível e do arquivo de BIOS necessário.
- O backend é intencionalmente minimalista e focado em carregar jogos, gerenciar o estado dos emuladores e entregar frames para a camada do console.