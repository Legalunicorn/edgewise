import { defineConfig } from 'vitepress'; // Configure the documentation site.

export default defineConfig({
  title: 'Edgewise', // Site title.
  description: 'Shared-memory graph algorithms in C++20',
  base: '/edgewise/', // Repository path for eventual GitHub Pages hosting.

  themeConfig: {
    search: {
      provider: 'local' // Enable browser-based fuzzy full-text search.
    },
    outline: [2, 3], // Include second- and third-level headings in page navigation.
    sidebar: [
      {
        text: 'Project', // General project documentation.
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Getting started', link: '/getting-started' },
          { text: 'Graph storage', link: '/graph-storage' }
        ]
      },
      {
        text: 'BFS', // Algorithm and implementation explanations.
        items: [
          { text: 'Sequential BFS', link: '/bfs/' },
          { text: 'Parallel implementations', link: '/bfs/implementations' }
        ]
      },
      {
        text: 'Benchmarks', // Measurement method and results.
        items: [
          { text: 'Methodology', link: '/benchmarks/methodology' },
          { text: 'Results', link: '/benchmarks/results' }
        ]
      }
    ]
  }
});
