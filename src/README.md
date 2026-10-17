# Código del simulador

main valida configuración; node conserva el arranque TCP e integra el bucle de
eventos; network codifica, recibe y encola mensajes; memory administra páginas locales;
protocol coordina permisos y transacciones; cli interpreta órdenes.

Ya no hay stubs en los módulos. Los contratos y flujos están documentados en
../docs/architecture/ y la explicación sencilla en ../docs/GUIA_SENCILLA.md.
