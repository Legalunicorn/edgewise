import { defineConfig } from 'vitepress'; // Configure the documentation site.

export default defineConfig({
    title: 'Edgewise', // Site title.
    description: 'Shared-memory graph algorithms in C++20',
    base: '/edgewise/', // Repository path for eventual GitHub Pages hosting.
    vite: {
        server: {
            watch: {
                usePolling: true, // Check files directly instead of relying on change notifications.
                interval: 500, // Check every half-second.
            },
        },
    },
    markdown: {
        math: true,
    },

    themeConfig: {
        socialLinks: [
            {
                icon: 'github', // Repository link in the page header.
                link: 'https://github.com/Legalunicorn/edgewise',
                ariaLabel: 'Edgewise on GitHub'
            }
        ],
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
                    { text: 'Graph storage', link: '/graph-storage.md' }
                ]
            },
            {
                text: 'BFS', // Algorithm and implementation explanations.
                items: [
                    { text: 'Introduction', link: '/bfs/' },
                    { text: 'Sequential BFS', link: '/bfs/sequential' },
                    { text: 'V1: Mutex discovery', link: '/bfs/v1-mutex' },
                    { text: 'V2: Atomic discovery', link: '/bfs/v2-atomic' },
                    { text: 'V3: Persistent workers', link: '/bfs/v3-persistent-workers' },
                    { text: 'V4: Parallel frontier assembly', link: '/bfs/v4-parallel-frontier-assembly' },
                    { text: 'Results', link: '/benchmarks/results' }
                ]
            },
            {
                text: 'PageRank',
                items: [
                    { text: 'Introduction', link: '/pagerank/' },
                    { text: 'Sequential PageRank', link: '/pagerank/sequential' },
                    { text: 'Parallel PageRank', link: '/pagerank/parallel' }
                ]
            },
            {
                text: 'SCC',
                items: [
                    { text: 'Introduction', link: '/scc/' }
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
